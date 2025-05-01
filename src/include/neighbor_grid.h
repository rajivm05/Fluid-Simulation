#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "particle.h"

class NeighborGrid {
private:
    float m_cellSize;       // Typically 2x smoothing length (h)
    uint32_t m_tableSize;   // Prime number for better distribution
    uint32_t* m_particleTable;
    std::vector<Particle*> m_sortedParticles;

    const glm::vec3 grid_pos;
    const float nx;
    const float ny;
    const float nz;
    const float h;

    float cell_size;
    std::vector<std::vector<Particle*>> particle_table;
    
public:
    uint32_t computeHash(const glm::ivec3& cell) const;

    NeighborGrid(float cell_sz, float smoothing_dist, float lx, float ly, float lz);
    // NeighborGrid(float cellSize, float smoothing_dist, uint32_t tableSize = 262144);
    // ~NeighborGrid();

    // Prevent copying
    NeighborGrid(const NeighborGrid&) = delete;
    NeighborGrid& operator=(const NeighborGrid&) = delete;

    void build(std::vector<Particle>& particles);
    void queryNeighbors(glm::vec3 pos, std::vector<Particle*>& neighbors) const;
    glm::ivec3 positionToCell(const glm::vec3& pos) const;
};
