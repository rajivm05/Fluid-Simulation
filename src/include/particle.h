#ifndef PARTICLE_H
#define PARTICLE_H

#include <vector>
#include <glm/glm.hpp>

#pragma pack(push, 1) // No padding
struct Particle_buffer {
    glm::vec3 position;
    glm::vec4 color;
    float density;
    glm::vec3 velocity;
    float pressure;
};
#pragma pack(pop)

struct Particle {
    glm::vec3 position;
    glm::vec4 color;   
    glm::vec3 velocity;
    glm::vec3 acceleration;
    float density;
    float pressure;
    uint32_t hash_value;

    std::vector<Particle*> neighbors;  

    Particle() = default;

    Particle(const Particle& p)
        : position(p.position),
          color(p.color),
          velocity(p.velocity),
          acceleration(p.acceleration),
          density(p.density),
          pressure(p.pressure),
          hash_value(p.hash_value),
          neighbors() {}

    Particle& operator=(const Particle& p) {
        position = p.position;
        color = p.color;   
        velocity = p.velocity;
        acceleration = p.acceleration;
        density = p.density;
        pressure = p.pressure;
        hash_value = p.hash_value;

        return *this;
    }

    Particle(Particle&&) = default;
    Particle& operator=(Particle&&) = default;
};

#endif
