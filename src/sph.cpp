#include <random>

#include "sph.h"

const glm::vec3 SPH::gravity(0.0f, -9.81f, 0.0f);

SPH::SPH(float dt, float df, int count, float lx, float ly, float lz, glm::vec4 box_color): delta_time(dt), damping_factor(df),
    lim_x(lx), lim_y(ly), lim_z(lz), buffer(count, Particle_buffer {}), box_color(box_color), num_threads(std::thread::hardware_concurrency()) {}

void SPH::initialize_particles(glm::vec3 center, float radius) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(0.0f, 1.0f);

    for(auto& b: buffer) {
        Particle p(b);

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
            // 0.5f + dist(gen) * 0.5f,
            // 0.5f + dist(gen) * 0.5f,
            // 0.5f + dist(gen) * 0.5f,

            62.0f / 255,
            164.0f / 255,
            240.0f / 255,
            0.8f
        );
        // p.density = rho0;

        particles.push_back(p);
    }
}

void SPH::initialize_particles_cube(glm::vec3 center, float side_length, float spacing) {
    particles.clear();  // optional: clear existing particles

    buffer.clear();

    int particles_per_axis = static_cast<int>(side_length / spacing);
    glm::vec3 start = center - glm::vec3(side_length) * 0.5f;

    buffer = std::vector<Particle_buffer>(particles_per_axis * particles_per_axis * particles_per_axis);

    for (int x = 0; x < particles_per_axis; ++x) {
        for (int y = 0; y < particles_per_axis; ++y) {
            for (int z = 0; z < particles_per_axis; ++z) {
                Particle p(buffer[x * particles_per_axis * particles_per_axis + y * particles_per_axis + z]);
                p.position = start + glm::vec3(x, y, z) * spacing;

                p.velocity = glm::vec3(0.0f);  // initial rest
                p.color = glm::vec4(
                    62.0f / 255.0f,
                    164.0f / 255.0f,
                    240.0f / 255.0f,
                    0.8f
                );
                p.density = rho0;

                particles.push_back(p);
            }
        }
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
    if(0 < r && r<=h){
        return ((float) viscosityLaplace_const * (h-r));
    }
    return 0.0f;
}

void SPH::update_properties(std::vector<Particle>::iterator begin, std::vector<Particle>::iterator end) {

    for(auto i = begin; i != end; i++) {
        auto& pi = *i;

        pi.density = 0.0f;
        // std::cout << "particle neighbors: \t" << pi.neighbors.size() << std::endl;

        // for(auto& pj_r: particles) {
        //algo
        //obtain hash value of cell of particle
        //iterate through all particles in the cell

        for(auto& pj:pi.neighbors){ 
            // Particle* pj = &pj_r;
            pi.density += mass * poly6(pi.position - pj->position, h);
            // std::cout << "Updating density: \t" << pi.density << std::endl;

        }
        // std::cout << "density: \t" << pi.density - rho0 << std::endl;

        pi.pressure = k * (pi.density - rho0);
    }
}

void SPH::calculate_forces(std::vector<Particle>::iterator begin, std::vector<Particle>::iterator end) {
    for(auto i = begin; i != end; i++) {
        auto& pi = *i;

        pi.acceleration = gravity;

        glm::vec3 pressure_force(0.0, 0.0, 0.0);
        glm::vec3 viscosity_force(0.0, 0.0, 0.0);

        // // for(auto& pj_r: particles) {
        //     // Particle* pj = &pj_r;
        for(auto& pj:pi.neighbors){ 
        
            if(pj->density == 0.0) { continue; }

            pressure_force -= mass * ((pi.pressure + pj->pressure) / (2 * pj->density)) * spiky_grad(pi.position - pj->position, h);
            viscosity_force += mu * mass * (pj->velocity - pi.velocity) * viscosity_laplace(pi.position - pj->position, h) / pj->density;

        }
        if(pi.density == 0){continue;}
        pi.acceleration += pressure_force / pi.density  ;
        pi.acceleration += viscosity_force / pi.density ;
        // std::cout << pi.neighbors.size()<< std::endl;

        pi.neighbors.clear();
    //     // std::cout << pressure_force.x / pi.density << std::endl;
    //     // std::cout << pi.density << std::endl;
    }
        // std::cout << "---------------------------------------------------------" << std::endl;

}

void SPH::update_state(std::vector<Particle>::iterator begin, std::vector<Particle>::iterator end) {
    for(auto i = begin; i != end; i++) {
        auto& p = *i;

        p.velocity += p.acceleration * delta_time;
        p.position += p.velocity * delta_time;

    }
}

void SPH::boundary_conditions(float sprite_size) {
    float flim_x = lim_x - sprite_size;
    float flim_y = lim_y - sprite_size;
    float flim_z = lim_z - sprite_size;
    for(auto& p: particles) {
        // if(p.outOfBox){
        //     if(p.position.y < -flim_y) {
        //         p.position.y = -flim_y;
        //         p.velocity.y = -p.velocity.y * damping_factor;
        //     }
        // }
        // else{
        //     if(p.position.y > flim_y + sprite_size && 
        //         (p.position.x < -flim_x ||
        //         p.position.x > flim_x ||
        //         p.position.z < -flim_z ||
        //         p.position.z > flim_z )) {
        //         // p.position.y = flim_y;
        //         // p.velocity.y = -p.velocity.y * damping_factor;
        //         p.outOfBox = true;
        //         continue;
        //     }
    
        //     if(p.position.x < -flim_x) {
        //         p.position.x = -flim_x;
        //         p.velocity.x = -p.velocity.x * damping_factor;
        //     }
    
        //     if(p.position.x > flim_x) {
        //         p.position.x = flim_x;
        //         p.velocity.x = -p.velocity.x * damping_factor;
        //     }
    
        //     if(p.position.y < -flim_y) {
        //         p.position.y = -flim_y;
        //         p.velocity.y = -p.velocity.y * damping_factor;
        //     }
        //     if(p.position.z < -flim_z) {
        //         p.position.z = -flim_z;
        //         p.velocity.z = -p.velocity.z * damping_factor;
        //     }
    
        //     if(p.position.z > flim_z) {
        //         p.position.z = flim_z;
        //         p.velocity.z = -p.velocity.z * damping_factor;
        //     }
        // }

        if(p.position.y < -flim_y) {
            p.position.y = -flim_y;
            p.velocity.y = -p.velocity.y * damping_factor;
        }
        if(p.position.x < -flim_x) {
            p.position.x = -flim_x;
            p.velocity.x = -p.velocity.x * damping_factor;
        }

        if(p.position.x > flim_x) {
            p.position.x = flim_x;
            p.velocity.x = -p.velocity.x * damping_factor;
        }

        if(p.position.z < -flim_z) {
            p.position.z = -flim_z;
            p.velocity.z = -p.velocity.z * damping_factor;
        }

        if(p.position.z > flim_z) {
            p.position.z = flim_z;
            p.velocity.z = -p.velocity.z * damping_factor;
        }
        if(p.position.y > flim_y ) {
            // p.position.y = flim_y;
            // p.velocity.y = -p.velocity.y * damping_factor;
        }


        
        
    }
}
void SPH::create_cuboid(){
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
    
        // // t5 - Right face
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
