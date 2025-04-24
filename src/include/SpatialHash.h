#pragma once
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include "particle.h" // Make sure this includes the Particle struct

class SpatialHash {
//     std::unordered_map<size_t, std::vector<Particle*>> grid;
//     float cellSize;
//     float h;


// public:
//     SpatialHash(float size, float h);

//     void update(std::vector<Particle>& particles);
//     void findNeighbors(Particle& p);

// private:
//     size_t hash(const glm::vec3& pos);

private:
    float m_cellSize;       // Typically 2x smoothing length (h)
    uint32_t m_tableSize;   // Prime number for better distribution
    uint32_t* m_particleTable;
    std::vector<Particle*> m_sortedParticles;
    
    // Hash computation
    
public:
    uint32_t computeHash(const glm::ivec3& cell) const;

    SpatialHash(float cellSize, uint32_t tableSize = 262144);
    ~SpatialHash();

    // Prevent copying
    SpatialHash(const SpatialHash&) = delete;
    SpatialHash& operator=(const SpatialHash&) = delete;

    void build(std::vector<Particle>& particles);
    void queryNeighbors(Particle& particle, float radius) const;
    glm::ivec3 positionToCell(const glm::vec3& pos) const;
};
