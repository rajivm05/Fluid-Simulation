#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "particle.h"

class SpatialHash {
private:
    float m_cellSize;       // Typically 2x smoothing length (h)
    uint32_t m_tableSize;   // Prime number for better distribution
    uint32_t* m_particleTable;
    std::vector<Particle*> m_sortedParticles;
    std::vector<Particle> m_sortedParticlesC;

    const float h;
    
public:
    uint32_t computeHash(const glm::ivec3& cell) const;

    SpatialHash(float smoothing_dist, uint32_t tableSize = 262144);
    ~SpatialHash();

    // Prevent copying
    SpatialHash(const SpatialHash&) = delete;
    SpatialHash& operator=(const SpatialHash&) = delete;

    void build(std::vector<Particle>& particles);
    void queryNeighbors(glm::vec3 pos, std::vector<Particle*>& neighbors);
    glm::ivec3 positionToCell(const glm::vec3& pos) const;
};
