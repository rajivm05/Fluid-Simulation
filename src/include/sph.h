#pragma once
#include <vector>
#include <thread>
#include <functional>
#include <glm/gtc/type_ptr.hpp>

#include "SpatialHash.h"
#include "particle.h"
#include "sph_consts.h"

class SPH {
public:
    const int num_threads;

    const float h;
    const float lim_x;
    const float lim_y;
    const float lim_z;
    const float sprite_size;

    const float delta_time = sph_c::delta_time;
    const float damping_factor = sph_c::damping_factor;
    const float mass = sph_c::mass;
    const float rho0 = sph_c::rho0;
    const float k = sph_c::k;
    const float mu = sph_c::mu; 

    const glm::vec4 box_color = sph_c::box_color;
    const glm::vec3 gravity = sph_c::gravity;

    const float poly6_const = 315 / (64 * glm::pi<float>() * glm::pow(h, 9));
    const float spikyGrad_const = -45 / (glm::pi<float>() * pow(h, 6));
    const float viscosityLaplace_const = 45 / (glm::pi<float>() * pow(h, 6));

    SpatialHash& sp_hash;

    std::vector<Particle> particles;
    std::vector<glm::vec3> box_positions;

    SPH(float smoothing_dist, float lx, float ly, float lz, float sp_size, SpatialHash& sh);

    void initialize_particles_sphere(int count, glm::vec3 center, float radius);
    void initialize_particles_cube(glm::vec3 center, float side_length, float spacing);

    void update_properties(std::vector<Particle>::iterator begin, std::vector<Particle>::iterator end);
    void calculate_forces(std::vector<Particle>::iterator begin, std::vector<Particle>::iterator end);
    void update_state(std::vector<Particle>::iterator begin, std::vector<Particle>::iterator end);
    void update_neighbors(std::vector<Particle>::iterator begin, std::vector<Particle>::iterator end);
    void boundary_conditions(std::vector<Particle>::iterator begin, std::vector<Particle>::iterator end);
    void create_cuboid();

    float poly6(glm::vec3 r_v, float h);
    glm::vec3 spiky_grad(glm::vec3 r_v, float h);
    float viscosity_laplace(glm::vec3 r_v, float h);

    template <typename Func, typename... Args>
    void parallel(Func&& func, Args&&... args) {
        int total = particles.size();
        int chunk = (total + num_threads - 1) / num_threads;

        std::vector<std::thread> threads;

        for(int i = 0; i < num_threads; i++) {
            auto begin = particles.begin() + i * chunk;
            auto end = particles.begin() + std::min((i + 1) * chunk, total);

            if(begin - particles.begin() >= total) { break; }

            threads.emplace_back(
                [this, begin, end, &func, &args...]() {
                    std::invoke(func, this, begin, end, std::forward<Args>(args)...);
                }
            );
        }

        for(auto& t: threads) { t.join(); }
    }

};

