#include "SpatialHash.h"
#include <cmath>




    SpatialHash::SpatialHash(float size, float h){
        this->cellSize = size;
        this->h = h;
    }
    
    size_t SpatialHash::hash(const glm::vec3& pos) {
        int x = static_cast<int>(pos.x / cellSize);
        int y = static_cast<int>(pos.y / cellSize);
        // int z = static_cast<int>(pos.z / cellSize);
        return ((size_t)x * 92837111) ^ ((size_t)y * 689287499);
        // return ()
    }
    
    void SpatialHash::update(std::vector<Particle>& particles) {
        grid.clear();
        for(auto& p : particles) {
            // grid[hash(p.position)].push_back(&p);
            size_t key = hash(p.position);
            grid[key].push_back(&p);
        }
    }
    
    void SpatialHash::findNeighbors(Particle& p) {
        p.neighbors.clear();
        for(int dx = -1; dx <= 1; dx++) {
            for(int dy = -1; dy <= 1; dy++) {
                glm::vec3 probe = p.position + glm::vec3(dx*cellSize, dy*cellSize, 0);
                // auto& cell = grid[hash(probe)];
                size_t key = hash(probe);
                if (grid.find(key) != grid.end()) {
                    for (Particle* neighbor : grid[key]) {
                        if (neighbor != &p && 
                            glm::distance(p.position, neighbor->position) < h) {
                            p.neighbors.push_back(neighbor);
                        }
                    }
                }
            }
        }
    }