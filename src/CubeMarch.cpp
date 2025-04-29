#include "CubeMarch.h"
#include <iostream>

CubeMarch::CubeMarch(float lim_x, float lim_y, float lim_z, float len, float smoothing_dist, SPH* sph_ptr, float iv, SpatialHash& sh):
                                                    len_cube(len),
                                                    nx(2 * lim_x / len + 1),
                                                    ny(2 * lim_y / len + 1),
                                                    nz(2 * lim_z / len + 1), 
                                                    // cells(std::vector(nx, std::vector(ny, std::vector(nz, CubeCell {})))),
                                                    cells(nx * ny * nz, CubeCell {}),
                                                    // num_threads(std::thread::hardware_concurrency()),
                                                    num_threads(1),
                                                    h(smoothing_dist),
                                                    sph(sph_ptr),
                                                    iso_value(iv), 
                                                    sp_hash(sh) {

    glm::vec3 trans(-lim_x, -lim_y, -lim_z);
    glm::mat4 trans_mat = glm::translate(glm::mat4(1.0f), trans);

    int iter = 0;
    for(int i = 0; i < nx; i++) {
        for(int j = 0; j < ny; j++) {
            for(int k = 0; k < nz; k++) {
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

void CubeMarch::update_neighbors(std::vector<CubeCell>::iterator begin, std::vector<CubeCell>::iterator end) {
    int k = 0;
    for(auto i = begin; i != end; i++) {
        auto& c = *i;

        c.neighbors.clear();
        sp_hash.queryNeighbors(c.position, c.neighbors);
        std::cout << k << " - " << c.neighbors.size() << std::endl;
        k++;
    }
}

void CubeMarch::load_triangles(const std::vector<glm::vec3>& loaded_triangles)
{
    this->triangles = loaded_triangles;
}

int CubeMarch::cube_index(int i, int j, int k) {
    return i * ny * nz + j * nz + k;
}

glm::vec3 CubeMarch::vertex_interpolation(float iso_value, int p1, int p2) {
    if(fabs(cells[p1].color - cells[p2].color) < 1e-6) { return cells[p1].position; }

    float mu = (iso_value - cells[p1].color) / (cells[p2].color - cells[p1].color);
    return cells[p1].position + mu * (cells[p2].position - cells[p1].position);
}

void CubeMarch::march_cubes(int begin, int end, std::vector<glm::vec3>& tris) {
    for(int i = begin; i < end; i++){
        for(int j = 0; j < ny - 1; j++){
            for(int k = 0; k < nz - 1; k++){
                int corners[8];
                for(int m =0; m<8; m++) {
                    int dx = m        & 1;
                    int dy = (m >> 1) & 1;
                    int dz = (m >> 2) & 1;
                    corners[m] = cube_index(i + dx, j + dy, k + dz);
                }

                int table_index = 0;
                for(int i = 0; i<8; i++){
                    if(cells[corners[i]].color > iso_value) { table_index |= (1 << i); }
                }
                
                int edgeList = edgeTable[table_index];
                if(edgeList == 0) { continue; }

                glm::vec3 vertList[12];
                if(edgeList & 1)
                    vertList[0] = vertex_interpolation(iso_value, corners[0], corners[1]);
                if(edgeList & 2)
                    vertList[1] = vertex_interpolation(iso_value, corners[1], corners[2]);
                if(edgeList & 4)
                    vertList[2] = vertex_interpolation(iso_value, corners[2], corners[3]);
                if(edgeList & 8)
                    vertList[3] = vertex_interpolation(iso_value, corners[3], corners[0]);
                if(edgeList & 16)
                    vertList[4] = vertex_interpolation(iso_value, corners[4], corners[5]);
                if(edgeList & 32)
                    vertList[5] = vertex_interpolation(iso_value, corners[5], corners[6]);
                if(edgeList & 64)
                    vertList[6] = vertex_interpolation(iso_value, corners[6], corners[7]);
                if(edgeList & 128)
                    vertList[7] = vertex_interpolation(iso_value, corners[7], corners[4]);
                if(edgeList & 256)
                    vertList[8] = vertex_interpolation(iso_value, corners[0], corners[4]);
                if(edgeList & 512)
                    vertList[9] = vertex_interpolation(iso_value, corners[1], corners[5]);
                if(edgeList & 1024)
                    vertList[10] = vertex_interpolation(iso_value, corners[2], corners[6]);
                if(edgeList & 2048)
                    vertList[11] = vertex_interpolation(iso_value, corners[3], corners[7]);

                int* triList = triTable[table_index];
                for(int i = 0; triList[i] != -1; i += 3){
                    glm::vec3 v0 = vertList[triList[i]];
                    glm::vec3 v1 = vertList[triList[i+1]];
                    glm::vec3 v2 = vertList[triList[i+2]];

                    tris.push_back(v0);
                    tris.push_back(v1);
                    tris.push_back(v2);
                }
            }
        }
    }
}

void CubeMarch::MarchingCubes() {
    int total = nx - 1;
    int chunk = (total + num_threads - 1) / num_threads;

    std::vector<std::thread> threads;
    std::vector<std::vector<glm::vec3>> local_triangles(num_threads, std::vector<glm::vec3>{});

    for(int i = 0; i < num_threads; i++) {
        int begin = i * chunk;
        int end = std::min((i + 1) * chunk, total);

        if(begin >= total) { break; }

        threads.emplace_back(
            [this, begin, end, i, &local_triangles]() {
                std::invoke(&CubeMarch::march_cubes, this, begin, end, local_triangles[i]);
            }
        );
    }

    for(auto& t: threads) { t.join(); }

    triangles.clear();
    for(auto& tris: local_triangles) {
        triangles.insert(triangles.end(), tris.begin(), tris.end());
    }
}
