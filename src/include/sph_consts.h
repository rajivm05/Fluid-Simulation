#pragma once

#include <glm/glm.hpp>

namespace main_c {
    extern const int width;
    extern const int height;
    extern const char* window_name;

    extern glm::vec3 cam_pos;
    extern glm::vec3 cam_target;
    extern glm::vec3 cam_up;
    extern float cam_fov;
    extern float cam_near;
    extern float cam_far;

    extern const float lim_x;  
    extern const float lim_y;
    extern const float lim_z;

    extern const float h;

    extern const int sphere_count;
    extern const glm::vec3 sphere_center;
    extern const float sphere_radius;

    extern const glm::vec3 cube_center;
    extern const float cube_side_length;
    extern const float cube_spacing;

    extern const float len_cube;
    extern const float iso_value;

    extern const float sprite_size;
}

namespace sph_c {
    extern const float delta_time;
    extern const float damping_factor;
    extern const float mass;
    extern const float rho0;
    extern const float k;
    extern const float mu; 

    extern const glm::vec3 gravity;

    extern const glm::vec4 box_color;
}
