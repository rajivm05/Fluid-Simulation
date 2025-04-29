#include <vector>
#include <thread>

#include "particle.h"
#include "sph.h"

struct CubeCell {
    glm::vec3 position;
    float color;
    std::vector<Particle*> neighbors;
};

struct Triangle{
    glm::vec3 v0;
    glm::vec3 v1;
    glm::vec3 v2;
};

class CubeMarch {
private:
    int num_threads;
    float h;

    SPH* sph;
    static int edgeTable[256];
    static int triTable[256][16];

    SpatialHash& sp_hash;

public:
    int nx;
    int ny;
    int nz;
    int iso_value;

    float len_cube;
    std::vector<CubeCell> cells;
    std::vector<glm::vec3> triangles;

    CubeMarch(float lim_x, float lim_y, float lim_z, float len, float smoothing_dist, SPH* sph_ptr, float iv, SpatialHash& sh);

    void MarchingCubes();
    void march_cubes(int begin, int end, std::vector<glm::vec3>& tris);
    int cube_index(int i, int j, int k);
    glm::vec3 vertex_interpolation(float iso_value, int p1, int p2);

    void update_color(std::vector<CubeCell>::iterator begin, std::vector<CubeCell>::iterator end);
    void update_neighbors(std::vector<CubeCell>::iterator begin, std::vector<CubeCell>::iterator end);
    void load_triangles(const std::vector<glm::vec3>& loaded_triangles);

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
