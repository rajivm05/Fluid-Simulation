#pragma once
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include "Particle.h" // Make sure this includes the Particle struct

class SpatialHash {
    std::unordered_map<size_t, std::vector<Particle*>> grid;
    float cellSize;
    float h;


public:
    SpatialHash(float size, float h);

    void update(std::vector<Particle>& particles);
    void findNeighbors(Particle& p);

private:
    size_t hash(const glm::vec3& pos);
};