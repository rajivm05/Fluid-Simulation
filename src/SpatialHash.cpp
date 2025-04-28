#include "SpatialHash.h"
#include <cmath>
#include <algorithm>

constexpr uint32_t NO_PARTICLE = 0xFFFFFFFF;

SpatialHash::SpatialHash(float smoothing_dist, uint32_t tableSize)
    : m_cellSize(smoothing_dist), m_tableSize(tableSize), m_particleTable(nullptr), h(smoothing_dist) {}

SpatialHash::~SpatialHash() {
    free(m_particleTable);
}

uint32_t SpatialHash::computeHash(const glm::ivec3& cell) const {
    return ((cell.x * 73856093) ^ 
            (cell.y * 19349663) ^ 
            (cell.z * 83492791)) % m_tableSize;
}

glm::ivec3 SpatialHash::positionToCell(const glm::vec3& pos) const {
    return {
        static_cast<int>(std::floor(pos.x / m_cellSize)),
        static_cast<int>(std::floor(pos.y / m_cellSize)),
        static_cast<int>(std::floor(pos.z / m_cellSize))
    };
}

void SpatialHash::build(std::vector<Particle>& particles) {
    m_sortedParticles.resize(particles.size());
    for(size_t i = 0; i < particles.size(); ++i) {
        m_sortedParticles[i] = &particles[i];
    }

    std::sort(m_sortedParticles.begin(), m_sortedParticles.end(),
        [this](const Particle* a, const Particle* b) {
            // return computeHash(positionToCell(a->position)) < computeHash(positionToCell(b->position));
            return a->hash_value < b->hash_value;

        });

    // 3. Build hash table
    m_particleTable = static_cast<uint32_t*>(malloc(m_tableSize * sizeof(uint32_t)));
    std::fill_n(m_particleTable, m_tableSize, NO_PARTICLE);

    uint32_t prevHash = NO_PARTICLE;
    for(uint32_t i = 0; i < m_sortedParticles.size(); ++i) {
        const uint32_t currentHash = computeHash(
            positionToCell(m_sortedParticles[i]->position));
        
        if(currentHash != prevHash) {
            m_particleTable[currentHash] = i;
            prevHash = currentHash;
        }
    }
}

void SpatialHash::queryNeighbors(
    glm::vec3 pos, std::vector<Particle*>& neighbors) const 
{
    float radius = h;

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
                    // if(glm::distance(pos, m_sortedParticles[i]->position) <= radius) {
                        neighbors.push_back(m_sortedParticles[i]);
                    // }
                    ++i;
                }
            }
        }
    }
}
