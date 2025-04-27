#include <vector>
#include <thread>

#include "particle.h"

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

    const float poly6_const = 315 / (64 * glm::pi<float>() * glm::pow(h, 9));

public:
    std::vector<std::vector<std::vector<CubeCell>>> cells;

    CubeMarch(float lim_x, float lim_y, float lim_z, float len, float smoothing_dist): len_cube(len),
                                                    nx(2 * lim_x / len + 1),
                                                    ny(2 * lim_y / len + 1),
                                                    nz(2 * lim_z / len + 1), 
                                                    cells(std::vector(nx, std::vector(ny, std::vector(nz, CubeCell {})))),
                                                    num_threads(std::thread::hardware_concurrency()),
                                                    h(smoothing_dist) {}

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
