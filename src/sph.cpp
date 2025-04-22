#include "sph.h"

const glm::vec3 SPH::gravity(0.0f, -9.81f, 0.0f);

SPH::SPH(float dt, float df, int count, int lx, int ly, int lz): delta_time(dt), damping_factor(df),
    lim_x(lx), lim_y(ly), lim_z(lz), particles(count, Particle {}) {}

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
            p.color = glm::vec4(1.0, 0.0, 0.0, 0.0);
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
