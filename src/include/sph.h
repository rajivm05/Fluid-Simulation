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

public:
    std::vector<Particle> particles;

    SPH(float dt, float df, int count, int lx, int ly, int lz);

    void initialize_particles(glm::vec3 center, float radius);
    void calculate_forces();
    void update_state();
    void boundary_conditions();
};

