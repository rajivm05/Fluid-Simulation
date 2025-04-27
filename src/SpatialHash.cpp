#include "SpatialHash.h"
#include <cmath>
#include <algorithm>




    // SpatialHash::SpatialHash(float size, float h){
    //     this->cellSize = size;
    //     this->h = h;
    // }
    
    // size_t SpatialHash::hash(const glm::vec3& pos) {
    //     int x = static_cast<int>(pos.x );
    //     int y = static_cast<int>(pos.y );
    //     int z = static_cast<int>(pos.z );
    //     return ((size_t)x * 92837111) ^ ((size_t)y * 689287499) ^ ((size_t)z*83492791);
    //     // return (size_t) x * 2 / cellSize + (size_t) y;  
    //     // return ()
    // }
    
    // void SpatialHash::update(std::vector<Particle>& particles) {
    //     grid.clear();
    //     for(auto& p : particles) {
    //         // grid[hash(p.position)].push_back(&p);
    //         size_t key = hash(p.position);
    //         grid[key].push_back(&p);
    //     }
    // }
    
    // void SpatialHash::findNeighbors(Particle& p) {
    //     p.neighbors.clear();
    //     for(int dx = -1; dx <= 1; dx++) {
    //         for(int dy = -1; dy <= 1; dy++) {
    //             for(int dz = -1; dz <=1; dz++){
    //                 glm::vec3 probe = p.position + glm::vec3(dx, dy, dz);
    //                 size_t key = hash(probe);
    //                 if (grid.find(key) != grid.end()) {
    //                     for (Particle* neighbor : grid[key]) {
    //                         if (neighbor != &p && 
    //                             glm::distance(p.position, neighbor->position) < h) {
    //                             p.neighbors.push_back(neighbor);
    //                         }
    //                     }
    //                 }

    //             }
                
    //         }
    //     }
    // }


constexpr uint32_t NO_PARTICLE = 0xFFFFFFFF;

SpatialHash::SpatialHash(float cellSize, uint32_t tableSize)
    : m_cellSize(cellSize), m_tableSize(tableSize), m_particleTable(nullptr) {}

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
    glm::vec3 pos, std::vector<Particle*>& neighbors, float radius) const 
{
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
    // return neighbors;
    // particle.neighbors = neighbors;
}
