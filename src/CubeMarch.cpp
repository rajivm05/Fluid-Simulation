#include "CubeMarch.h"

CubeMarch::CubeMarch(float lim_x, float lim_y, float lim_z, float len, float smoothing_dist, SPH* sph_ptr):
                                                    len_cube(len),
                                                    nx(2 * lim_x / len + 1),
                                                    ny(2 * lim_y / len + 1),
                                                    nz(2 * lim_z / len + 1), 
                                                    // cells(std::vector(nx, std::vector(ny, std::vector(nz, CubeCell {})))),
                                                    cells(nx * ny * nz, CubeCell {}),
                                                    num_threads(std::thread::hardware_concurrency()),
                                                    h(smoothing_dist),
                                                    sph(sph_ptr) {

    glm::vec3 trans(-lim_x, -lim_y, -lim_z);
    glm::mat4 trans_mat = glm::translate(glm::mat4(1.0f), trans);

    int iter = 0;
    for(int i = 0; i < nx; i++) {
        for(int j = 0; j < ny; j++) {
            for(int k = 0; k < nx; k++) {
                cells[iter].position = (trans_mat * glm::vec4(i * len_cube, j * len_cube, k * len_cube, 1.0f));
                iter++;
            }
        }
    }
}

void CubeMarch::update_color(std::vector<CubeCell>::iterator begin, std::vector<CubeCell>::iterator end) {
    for(auto i = begin; i != end; i++) {
        auto& c = *i;

        c.color = 0.0f;
        for(auto p: c.neighbors) {
            c.color += sph->mass / p->density * sph->poly6(c.position - p->position, h);
        }
    }
}
