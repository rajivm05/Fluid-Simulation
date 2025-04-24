#ifndef PARTICLE_H
#define PARTICLE_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"

// Particle struct with position, velocity, and color
struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;
    glm::vec3 acceleration;
    float density;
    float pressure;
    uint32_t hash_value;

    std::vector<Particle*> neighbors;  

};
#endif
