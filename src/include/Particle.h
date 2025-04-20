#ifndef PARTICLE_H
#define PARTICLE_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <random>
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

    //added constraints
    const float h = 0.1f;       // Smoothing radius
    const float mass = 0.02f;   // Particle mass
    const float rho0 = 1000.0f; // Reference density
    const float k = 1000.0f;    // Pressure stiffness
    const float mu = 0.1f;      // Viscosity

    std::vector<Particle*> neighbors;  

};
#endif