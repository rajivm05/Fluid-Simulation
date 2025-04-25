#include <vector>
#include <thread>
#include <functional>
#include "particle.h"

class SPH {
private:
    float delta_time;
    float damping_factor;

    float lim_x;
    float lim_y;
    float lim_z;

    static const glm::vec3 gravity;

    int num_threads;
public:
    std::vector<Particle> particles;
    std::vector<Particle_buffer> buffer;
    std::vector<glm::vec3> box_positions;
    const glm::vec4 box_color;

    const float h = 0.06f;       // Smoothing radius
    const float mass = 0.05f;   // Particle mass
    const float rho0 = 1000.0f; // Reference density
    //working constants
    const float k = 1.0f;    // Pressure stiffness
    const float mu = 10.5f; 

    //experimental constants
    // const float k = 2000.0f;    // Pressure stiffness
    // const float mu = 0.5f; 

    const float poly6_const = 315 / (64 * glm::pi<float>() * glm::pow(h, 9));
    const float spikyGrad_const = -45 / (glm::pi<float>() * pow(h, 6));
    const float viscosityLaplace_const = 45/(glm::pi<float>() * pow(h, 6));

    SPH(float dt, float df, int count, float lx, float ly, float lz, glm::vec4 box_color);

    void initialize_particles(glm::vec3 center, float radius);
    void update_properties(std::vector<Particle>::iterator begin, std::vector<Particle>::iterator end);
    void calculate_forces(std::vector<Particle>::iterator begin, std::vector<Particle>::iterator end);
    void update_state(std::vector<Particle>::iterator begin, std::vector<Particle>::iterator end);
    void boundary_conditions(float sprite_size);
    void create_cuboid();
    void initialize_particles_cube(glm::vec3 center, float side_length, float spacing);

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

