#include "neighbor_grid.h"
#include <cmath>
#include <algorithm>

constexpr uint32_t NO_PARTICLE = 0xFFFFFFFF;

NeighborGrid::NeighborGrid(float cell_sz, float smoothing_dist, float lx, float ly, float lz)
    : cell_size(cell_sz), h(smoothing_dist), grid_pos(-lx, -ly, -lz), nx(2 * lx / cell_sz + 1), ny(2 * ly / cell_sz + 1),
    nz(2 * lz / cell_sz + 1), particle_table(nx * ny * nz, std::vector<Particle*> {}) {}

uint32_t NeighborGrid::computeHash(const glm::ivec3& cell) const {
    return cell.x * ny * nz + cell.y * nz + cell.z;
}

glm::ivec3 NeighborGrid::positionToCell(const glm::vec3& pos) const {
    return { (pos - grid_pos) / cell_size };
}

void NeighborGrid::build(std::vector<Particle>& particles) {
    for(auto& v: particle_table) { v.clear(); }

    for(auto& p: particles) {
        particle_table[p.hash_value].push_back(&p);
    }
}

void NeighborGrid::queryNeighbors(
    glm::vec3 pos, std::vector<Particle*>& neighbors) const 
{
    float radius = 2 * h;

    const glm::ivec3 baseCell = positionToCell(pos);
    const int searchRadius = static_cast<int>(std::ceil(radius / m_cellSize));

    for(int dx = -searchRadius; dx <= searchRadius; ++dx) {
        for(int dy = -searchRadius; dy <= searchRadius; ++dy) {
            for(int dz = -searchRadius; dz <= searchRadius; ++dz) {
                const glm::ivec3 cell = baseCell + glm::ivec3(dx, dy, dz);
                const uint32_t hash = computeHash(cell);

                if(m_particleTable[hash] == NO_PARTICLE) continue;

                // Find particles in this cell
                uint32_t i = m_particleTable[hash];
                while(i < m_sortedParticles.size() && 
                      computeHash(positionToCell(m_sortedParticles[i]->position)) == hash) 
                {
                    if(glm::distance(pos, m_sortedParticles[i]->position) <= radius) {
                        neighbors.push_back(m_sortedParticles[i]);
                    }
                    ++i;
                }
            }
        }
    }
}
