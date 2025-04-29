#include "sph_consts.h"

#include <glm/glm.hpp>

namespace main_c {

    // Screen Constants
    int width = 800;
    int height = 600;
    const char* window_name = "Particles Array";

    const float l = 0.5f;

    // Camera Constants
    glm::vec3 cam_pos(2*l, 1.5*l, 4.5*l);
    glm::vec3 cam_target(0.0f, 0.0f, 0.0f);
    glm::vec3 cam_up(0.0f, 1.0f, 0.0f);
    float cam_fov(glm::radians(45.0f));
    float cam_near = 0.1f;
    float cam_far = 100.0f;

    // SPH Constants
    const float lim_x = l;  
    const float lim_y = 0.5*l;
    const float lim_z = l;

    // Smoothing Distance
    const float h = 0.06f;

    // Sphere Particle Initialization
    const int sphere_count = 5000;
    const glm::vec3 sphere_center(0.0f, 0.0f, 0.0f);
    const float sphere_radius = l / 4;

    // Cube Particle Initialization
    const glm::vec3 cube_center(-l/2, l, 0.0f);
    const float cube_side_length = l / 1.5;
    const float cube_spacing = h / 3.5f;

    // CM Constants
    const float cm_h = h;
    const float len_cube = h / 4.0f;
    const float iso_value = 0.6;

    const float sprite_size = l / 8;
}

namespace sph_c {
    const float delta_time = 0.016f;
    const float damping_factor = 0.3;
    const float mass = 0.05f;
    const float rho0 = 1000.0f;
    const float k = 1.0f;
    const float mu = 1.5f; 

    const glm::vec3 gravity(0.0f, -9.81f, 0.0f);

    const glm::vec4 box_color = glm::vec4(0.0, 0.0, 0.0, 0.2);
}
