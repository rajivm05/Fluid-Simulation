#ifndef PARTICLE_H
#define PARTICLE_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"

#pragma pack(push, 1) // No padding
struct Particle_buffer {
    glm::vec3 position;
    glm::vec4 color;
    float density;
    glm::vec3 velocity;
    float pressure;
};
#pragma pack(pop)


// Particle struct with position, velocity, and color
struct Particle {
    glm::vec3& position;
    glm::vec4& color;   
    glm::vec3& velocity;
    glm::vec3 acceleration;
    float& density;
    float& pressure;
    uint32_t hash_value;
    bool outOfBox = false;

    std::vector<Particle*> neighbors;  

    Particle(Particle_buffer& pb): position(pb.position), color(pb.color), density(pb.density), pressure(pb.pressure), velocity(pb.velocity) {}
};
#endif
