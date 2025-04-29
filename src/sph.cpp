#include <random>

#include "sph.h"

SPH::SPH(float smoothing_dist, float lx, float ly, float lz, float sp_size, SpatialHash& sh): h(smoothing_dist),
    lim_x(lx), lim_y(ly), lim_z(lz), sprite_size(sp_size), sp_hash(sh), num_threads(std::thread::hardware_concurrency()) {}

void SPH::initialize_particles_sphere(int count, glm::vec3 center, float radius) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(0.0f, 1.0f);

    particles = std::vector<Particle>(count);

    for(auto& p: particles) {
        float r = radius * std::cbrt(dist(gen));
        float theta = 2.0f * glm::pi<float>() * dist(gen);
        float phi = std::acos(1.0f - 2.0f * dist(gen));

        p.position = glm::vec3(
            r * std::sin(phi) * std::cos(theta),
            r * std::sin(phi) * std::sin(theta),
            r * std::cos(phi)
        );

        p.velocity = glm::vec3(
            // (1.0f - 2.0f * dist(gen)) * 0.5,
            // (1.0f - 2.0f * dist(gen)) * 0.5,
            // (1.0f - 2.0f * dist(gen)) * 0.5
            0, 0, 0
        );

        p.color = glm::vec4(
            62.0f / 255,
            164.0f / 255,
            240.0f / 255,
            0.8f
        );
    }
}

void SPH::initialize_particles_cube(glm::vec3 center, float side_length, float spacing) {
    int particles_per_axis = static_cast<int>(side_length / spacing);
    glm::vec3 start = center - glm::vec3(side_length) * 0.5f;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(0.5f, 1.0f);

    particles = std::vector<Particle>(particles_per_axis * particles_per_axis * particles_per_axis);

    for (int x = 0; x < particles_per_axis; ++x) {
        for (int y = 0; y < particles_per_axis; ++y) {
            for (int z = 0; z < particles_per_axis; ++z) {
                Particle& p = particles[x * particles_per_axis * particles_per_axis + y * particles_per_axis + z];
                p.position = start + glm::vec3(x, y, z) * spacing;

                // p.velocity = glm::vec3(0.0f);  // initial rest
                p.velocity = glm::vec3(0.01 * dist(gen), 0.01 * dist(gen), 0.01 * dist(gen));
                p.color = glm::vec4(
                    62.0f / 255.0f,
                    164.0f / 255.0f,
                    240.0f / 255.0f,
                    0.8f
                );
            }
        }
    }
}

void SPH::update_hash(std::vector<Particle>::iterator begin, std::vector<Particle>::iterator end) {
    for(auto i = begin; i != end; i++) {
        auto& p = *i;
        p.hash_value = sp_hash.computeHash(sp_hash.positionToCell(p.position));
    }
}

float SPH::poly6(glm::vec3 r_v, float h) {
    float r = glm::length(r_v);
    if(r <= h) {
        return poly6_const * pow((pow(h, 2) - pow(r, 2)), 3);
    }

    return 0;
}

glm::vec3 SPH::spiky_grad(glm::vec3 r_v, float h) {
    float r = glm::length(r_v);
    if(0 < r && r <= h) {
        return ((float) (spikyGrad_const * pow((h - r), 2) / r)) * r_v;
    }

    return glm::vec3(0);
}

float SPH::viscosity_laplace(glm::vec3 r_v, float h) {
    float r = glm::length(r_v);
    if(0 < r && r <= h) {
        return ((float) viscosityLaplace_const * (h - r));
    }

    return 0.0f;
}

void SPH::update_properties(std::vector<Particle>::iterator begin, std::vector<Particle>::iterator end) {
    for(auto i = begin; i != end; i++) {
        auto& pi = *i;

        pi.density = 0.0f;
        for(auto& pj: pi.neighbors){ 
            pi.density += mass * poly6(pi.position - pj->position, h);
        }

        pi.pressure = k * (pi.density - rho0);
    }
}

void SPH::calculate_forces(std::vector<Particle>::iterator begin, std::vector<Particle>::iterator end) {
    for(auto i = begin; i != end; i++) {
        auto& pi = *i;
        if(pi.density == 0) { continue; }

        glm::vec3 pressure_force(0.0, 0.0, 0.0);
        glm::vec3 viscosity_force(0.0, 0.0, 0.0);

        for(auto& pj: pi.neighbors){ 
            if(pj->density == 0.0) { continue; }

            pressure_force -= mass * ((pi.pressure + pj->pressure) / (2 * pj->density)) * spiky_grad(pi.position - pj->position, h);
            viscosity_force += mu * mass * (pj->velocity - pi.velocity) / pj->density * viscosity_laplace(pi.position - pj->position, h);

        }

        pi.acceleration = gravity;
        pi.acceleration += pressure_force / pi.density  ;
        pi.acceleration += viscosity_force / pi.density ;
    }
}

void SPH::update_state(std::vector<Particle>::iterator begin, std::vector<Particle>::iterator end) {
    for(auto i = begin; i != end; i++) {
        auto& p = *i;

        p.velocity += p.acceleration * delta_time;
        p.position += p.velocity * delta_time;
    }
}

void SPH::update_neighbors(std::vector<Particle>::iterator begin, std::vector<Particle>::iterator end) {
    for(auto i = begin; i != end; i++) {
        auto& p = *i;

        p.neighbors.clear();
        sp_hash.queryNeighbors(p.position, p.neighbors);
    }
}

void SPH::boundary_conditions(std::vector<Particle>::iterator begin, std::vector<Particle>::iterator end) {
    float flim_x = lim_x - sprite_size / 2;
    float flim_y = lim_y - sprite_size / 2;
    float flim_z = lim_z - sprite_size / 2;

    for(auto i = begin; i != end; i++) {
        auto& p = *i;
        if(p.position.x < -flim_x) {
            p.position.x = -flim_x;
            p.velocity.x = -p.velocity.x * damping_factor;
        }

        if(p.position.x > flim_x) {
            p.position.x = flim_x;
            p.velocity.x = -p.velocity.x * damping_factor;
        }

        // if(p.position.y > flim_y) {
        //     p.position.y = flim_y;
        //     p.velocity.y = -p.velocity.y * damping_factor;
        // }

        if(p.position.y < -flim_y) {
            p.position.y = -flim_y;
            p.velocity.y = -p.velocity.y * damping_factor;
        }

        if(p.position.z < -flim_z) {
            p.position.z = -flim_z;
            p.velocity.z = -p.velocity.z * damping_factor;
        }

        if(p.position.z > flim_z) {
            p.position.z = flim_z;
            p.velocity.z = -p.velocity.z * damping_factor;
        }
    }
}

void SPH::create_cuboid() {
    box_positions = {
        // t1 - Left face
        glm::vec3(-lim_x, -lim_y,  lim_z),
        glm::vec3(-lim_x,  lim_y,  lim_z),
        glm::vec3(-lim_x,  lim_y, -lim_z),
    
        // t2 - Left face
        glm::vec3(-lim_x, -lim_y,  lim_z),
        glm::vec3(-lim_x, -lim_y, -lim_z),
        glm::vec3(-lim_x,  lim_y, -lim_z),
    
        // t3 - Bottom face
        glm::vec3(-lim_x, -lim_y,  lim_z),
        glm::vec3(-lim_x, -lim_y, -lim_z),
        glm::vec3( lim_x, -lim_y,  lim_z),
    
        // t4 - Bottom face
        glm::vec3(-lim_x, -lim_y, -lim_z),
        glm::vec3( lim_x, -lim_y,  lim_z),
        glm::vec3( lim_x, -lim_y, -lim_z),
    
        // t5 - Right face
        glm::vec3( lim_x, -lim_y,  lim_z),
        glm::vec3( lim_x,  lim_y,  lim_z),
        glm::vec3( lim_x,  lim_y, -lim_z),
    
        // t6 - Right face
        glm::vec3( lim_x, -lim_y,  lim_z),
        glm::vec3( lim_x, -lim_y, -lim_z),
        glm::vec3( lim_x,  lim_y, -lim_z),
    
        // t7 - Back face
        glm::vec3(-lim_x, -lim_y, -lim_z),
        glm::vec3(-lim_x,  lim_y, -lim_z),
        glm::vec3( lim_x,  lim_y, -lim_z),
    
        // t8 - Back face
        glm::vec3(-lim_x, -lim_y, -lim_z),
        glm::vec3( lim_x, -lim_y, -lim_z),
        glm::vec3( lim_x,  lim_y, -lim_z),
    
        // t9 - Top face
        glm::vec3(-lim_x,  lim_y,  lim_z),
        glm::vec3(-lim_x,  lim_y, -lim_z),
        glm::vec3( lim_x,  lim_y,  lim_z),
    
        // t10 - Top face
        glm::vec3(-lim_x,  lim_y, -lim_z),
        glm::vec3( lim_x,  lim_y,  lim_z),
        glm::vec3( lim_x,  lim_y, -lim_z),

        // t11 - Front face
        glm::vec3(-lim_x, -lim_y,  lim_z),
        glm::vec3(-lim_x,  lim_y,  lim_z),
        glm::vec3( lim_x,  lim_y,  lim_z),

        // t12 - Front face
        glm::vec3(-lim_x, -lim_y,  lim_z),
        glm::vec3( lim_x,  lim_y,  lim_z),
        glm::vec3( lim_x, -lim_y,  lim_z),
    };
}
