#include <vector>
#include <thread>

#include "particle.h"
#include "sph.h"

struct CubeCell {
    glm::vec3 position;
    float color;
    std::vector<Particle*> neighbors;
};

class CubeMarch {
private:
    int nx;
    int ny;
    int nz;

    float len_cube;

    int num_threads;
    float h;

    SPH* sph;

public:
    // std::vector<std::vector<std::vector<CubeCell>>> cells;
    std::vector<CubeCell> cells;

    CubeMarch(float lim_x, float lim_y, float lim_z, float len, float smoothing_dist, SPH* sph_ptr);
    void update_color(std::vector<CubeCell>::iterator begin, std::vector<CubeCell>::iterator end);

    template <typename Func, typename... Args>
    void parallel(Func&& func, Args&&... args) {
        int total = cells.size();
        int chunk = (total + num_threads - 1) / num_threads;

        std::vector<std::thread> threads;

        for(int i = 0; i < num_threads; i++) {
            auto begin = cells.begin() + i * chunk;
            auto end = cells.begin() + std::min((i + 1) * chunk, total);

            if(begin - cells.begin() >= total) { break; }

            threads.emplace_back(
                [this, begin, end, &func, &args...]() {
                    std::invoke(func, this, begin, end, std::forward<Args>(args)...);
                }
            );
        }

        for(auto& t: threads) { t.join(); }
    }
};
