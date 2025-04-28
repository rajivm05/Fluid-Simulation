#ifndef FRAME_H
#define FRAME_H
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"
#pragma pack(push, 1) // No padding
struct FrameHeader {
    char magic[4] = {'S','P','H'}; // Identifier
    uint32_t version = 3;          // Format version
    double timestamp;              // Simulation time
    uint32_t particle_count;       // For validation
    uint32_t triangle_count;       // For validation
    float h;                       // Smoothing radius
    float dt;                      // Timestep
    glm::mat4 view;                // Camera matrices
    glm::mat4 projection;
    glm::vec3 gravity;             // Simulation params
    float damping_factor;
    glm::vec4 box_limits;          // x=lim_x, y=lim_y, z=lim_z, w=scale
    float cube_len;
    float iso_value;
};
#pragma pack(pop)
#endif
