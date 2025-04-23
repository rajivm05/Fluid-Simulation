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

void SPH::calculate_forces() {
    for(auto& p: particles) {
        p.acceleration = gravity;

        if(p.neighbors.size() > 0) {
            p.color = glm::vec4(1.0, 0.0, 0.0, 1.0);
        } else {
            p.color = glm::vec4(
                62.0f/255,
                164.0f/255,
                240.0f/255,
                0.8f
            );
        }
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
    // box_positions = {
    //     //t1
    //     glm::vec3(-lim_x, -lim_y, lim_z),
    //     glm::vec3(-lim_x, lim_y, lim_z),
    //     glm::vec3(-lim_x, lim_y, -lim_z),

    //     //t2
    //     glm::vec3(-lim_x, -lim_y, lim_z),
    //     glm::vec3(-lim_x, -lim_y, -lim_z),
    //     glm::vec3(-lim_x, lim_y, -lim_z),

    //     //t3
    //     glm::vec3(-lim_x, -lim_y, lim_z),
    //     glm::vec3(-lim_x, -lim_y, -lim_z),
    //     glm::vec3(lim_x, -lim_y, lim_z),

    //     //t4
    //     glm::vec3(-lim_x, -lim_y, -lim_z),
    //     glm::vec3(lim_x, -lim_y, lim_z),
    //     glm::vec3(lim_x, -lim_y, -lim_z),

    //     //t5

    // };
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
    
        // Note: front face (z = +lim_z) is omitted intentionally (open box)
    };
    
}
