#include <random>

#include "sph.h"

const glm::vec3 SPH::gravity(0.0f, -9.81f, 0.0f);

SPH::SPH(float dt, float df, int count, int lx, int ly, int lz, glm::vec4 box_color): delta_time(dt), damping_factor(df),
    lim_x(lx), lim_y(ly), lim_z(lz), particles(count, Particle {}), box_color(box_color) {}

void SPH::initialize_particles(glm::vec3 center, float radius) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(0.0f, 1.0f);

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
            (1.0f - 2.0f * dist(gen)) * 10,
            (1.0f - 2.0f * dist(gen)) * 10,
            (1.0f - 2.0f * dist(gen)) * 10
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
    }
}

float poly6(glm::vec3 r_v, float h) {
    float r = glm::length(r_v);
    if(r <= h) {
        return 315 / (64 * glm::pi<float>() * glm::pow(h, 9)) * pow((pow(h, 2) - pow(r, 2)), 3);
    }

    return 0;
}

glm::vec3 spiky_grad(glm::vec3 r_v, float h) {
    float r = glm::length(r_v);
    if(0 < r && r <= h) {
        return ((float) (-45 / (glm::pi<float>() * pow(h, 6)) * pow((h - r), 2) / r)) * r_v;
    }

    return glm::vec3(0);
}

float vanessa_laplace(glm::vec3 r_v, float h) {
    float r = glm::length(r_v);
}

void SPH::update_properties() {
    for(auto& pi: particles) {
        pi.density = 0;

        for(auto& pj_r: particles) {
            Particle* pj = &pj_r;
            pi.density += pj->mass * poly6(pi.position - pj->position, pi.h);
        }

        pi.pressure = pi.k * (pi.density - pi.rho0);
    }
}

void SPH::calculate_forces() {
    update_properties();

    for(auto& pi: particles) {
        pi.acceleration = gravity;

        glm::vec3 pressure_force(0.0, 0.0, 0.0);

        for(auto& pj_r: particles) {
            Particle* pj = &pj_r;
            if(pj->density == 0.0) { continue; }

            pressure_force -= pj->mass * ((pi.pressure + pj->pressure) / (2 * pj->density)) * spiky_grad(pi.position - pj->position, pi.h);
        }

        pi.acceleration += pressure_force / pi.density * 0.001f;
        // std::cout << pressure_force.x / pi.density << std::endl;
        // std::cout << pi.density << std::endl;

        /*
        if(p.neighbors.size() > 0) {
            p.color = glm::vec4(1.0, 0.0, 0.0, 1.0);
        } else {
            p.color = glm::vec4(
                62.0f/255,
                164.0f/255,
                240.0f/255,
                0.8f
            );
        }*/
    }
}

void SPH::update_state() {
    for(auto& p: particles) {
        p.velocity += p.acceleration * delta_time;
        p.position += p.velocity * delta_time;
    }
}

void SPH::boundary_conditions() {
    for(auto& p: particles) {
        if(p.position.x < -lim_x) {
            p.position.x = -lim_x;
            p.velocity.x = -p.velocity.x * damping_factor;
        }

        if(p.position.x > lim_x) {
            p.position.x = lim_x;
            p.velocity.x = -p.velocity.x * damping_factor;
        }

        if(p.position.y < -lim_y) {
            p.position.y = -lim_y;
            p.velocity.y = -p.velocity.y * damping_factor;
        }

        if(p.position.y > lim_y) {
            p.position.y = lim_y;
            p.velocity.y = -p.velocity.y * damping_factor;
        }

        if(p.position.z < -lim_z) {
            p.position.z = -lim_z;
            p.velocity.z = -p.velocity.z * damping_factor;
        }

        if(p.position.z > lim_z) {
            p.position.z = lim_z;
            p.velocity.z = -p.velocity.z * damping_factor;
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
        // glm::vec3( lim_x, -lim_y,  lim_z),
        // glm::vec3( lim_x,  lim_y,  lim_z),
        // glm::vec3( lim_x,  lim_y, -lim_z),
    
        // t6 - Right face
        // glm::vec3( lim_x, -lim_y,  lim_z),
        // glm::vec3( lim_x, -lim_y, -lim_z),
        // glm::vec3( lim_x,  lim_y, -lim_z),
    
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
    };
    
}
