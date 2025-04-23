#include <vector>
#include "particle.h"

class SPH {
private:
    float delta_time;
    float damping_factor;

    int lim_x;
    int lim_y;
    int lim_z;

    static const glm::vec3 gravity;

    void update_properties();
    float viscocity_laplace();

public:
    std::vector<Particle> particles;
    std::vector<glm::vec3> box_positions;
    const glm::vec4 box_color;

    SPH(float dt, float df, int count, int lx, int ly, int lz, glm::vec4 box_color);

    void initialize_particles(glm::vec3 center, float radius);
    void calculate_forces();
    void update_state();
    void boundary_conditions();
    void create_cuboid();
};

