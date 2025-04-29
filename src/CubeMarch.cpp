#include "CubeMarch.h"
#include <iostream>

CubeMarch::CubeMarch(float lim_x, float lim_y, float lim_z, float len, float smoothing_dist, SPH* sph_ptr, float iv, SpatialHash& sh):
                                                    len_cube(len),
                                                    nx(2 * lim_x / len + 1),
                                                    ny(2 * lim_y / len + 1),
                                                    nz(2 * lim_z / len + 1), 
                                                    cells(nx * ny * nz, CubeCell {}),
                                                    num_threads(std::thread::hardware_concurrency()),
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
            if(p->density == 0.0) { continue; }

            c.color += sph->mass / p->density * sph->poly6(c.position - p->position, h);
        }
    }
}

void CubeMarch::update_neighbors(std::vector<CubeCell>::iterator begin, std::vector<CubeCell>::iterator end) {
    for(auto i = begin; i != end; i++) {
        auto& c = *i;

        c.neighbors.clear();
        sp_hash.queryNeighbors(c.position, c.neighbors);
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
    const glm::vec3 v1 = cells[p1].position;
    const glm::vec3 v2 = cells[p2].position;

    const float c1 = cells[p1].color;
    const float c2 = cells[p2].color;

    if(std::abs(iso_value - c1) < 1e-5) { return v1; }
    if(std::abs(iso_value - c2) < 1e-5) { return v2; }
    if(std::abs(c1 - c2) < 1e-5) { return v1; }

    float mu = (iso_value - c1) / (c2 - c1);
    return v1 + mu * (v2 - v1);
}

// void CubeMarch::march_cubes(int begin, int end, std::vector<glm::vec3>& tris) {
void CubeMarch::march_cubes(int begin, int end, std::unordered_map<Edge, std::pair<glm::vec3, glm::vec3>, EdgeHash>& local_map_i, std::vector<Edge>& tris) {
    for(int i = begin; i < end; i++){
        for(int j = 0; j < ny - 1; j++){
            for(int k = 0; k < nz - 1; k++){
                int corners[8];
                corners[0] = cube_index(i    , j    , k    );
                corners[1] = cube_index(i + 1, j    , k    );
                corners[2] = cube_index(i + 1, j    , k + 1);
                corners[3] = cube_index(i    , j    , k + 1);
                corners[4] = cube_index(i    , j + 1, k    );
                corners[5] = cube_index(i + 1, j + 1, k    );
                corners[6] = cube_index(i + 1, j + 1, k + 1);
                corners[7] = cube_index(i    , j + 1, k + 1);

                int table_index = 0;
                for(int m = 0; m < 8; m++){
                    if(cells[corners[m]].color > iso_value) { table_index |= (1 << m); }
                }

                int edgeList = edgeTable[table_index];
                if(edgeList == 0) { continue; }

                // if(edgeList &    1) { vertList[0]  = vertex_interpolation(iso_value, corners[0], corners[1]); }
                // if(edgeList &    2) { vertList[1]  = vertex_interpolation(iso_value, corners[1], corners[2]); }
                // if(edgeList &    4) { vertList[2]  = vertex_interpolation(iso_value, corners[2], corners[3]); }
                // if(edgeList &    8) { vertList[3]  = vertex_interpolation(iso_value, corners[3], corners[0]); }
                // if(edgeList &   16) { vertList[4]  = vertex_interpolation(iso_value, corners[4], corners[5]); }
                // if(edgeList &   32) { vertList[5]  = vertex_interpolation(iso_value, corners[5], corners[6]); }
                // if(edgeList &   64) { vertList[6]  = vertex_interpolation(iso_value, corners[6], corners[7]); }
                // if(edgeList &  128) { vertList[7]  = vertex_interpolation(iso_value, corners[7], corners[4]); }
                // if(edgeList &  256) { vertList[8]  = vertex_interpolation(iso_value, corners[0], corners[4]); }
                // if(edgeList &  512) { vertList[9]  = vertex_interpolation(iso_value, corners[1], corners[5]); }
                // if(edgeList & 1024) { vertList[10] = vertex_interpolation(iso_value, corners[2], corners[6]); }
                // if(edgeList & 2048) { vertList[11] = vertex_interpolation(iso_value, corners[3], corners[7]); }
                
                glm::vec3 vertList[12];
                Edge edgeSave[12];
                for(int m = 0; m < 12; m++) {
                    if(edgeList & (1 << m)) {
                        Edge e {corners[edgeMap[m][0]], corners[edgeMap[m][1]]};
                        edgeSave[m] = e;

                        auto f = local_map_i.find(e);
                        if(f != local_map_i.end()) {
                            vertList[m] = f->second.first;
                        } else {
                            glm::vec3 interp = vertex_interpolation(iso_value, e.v1, e.v2);
                            local_map_i.insert({e, {interp, glm::vec3(0.0f)}});
                            vertList[m] = interp;
                        }
                    }
                }

                int* triList = triTable[table_index];
                for(int m = 0; triList[m] != -1; m += 3){
                    glm::vec3 v0 = vertList[triList[m  ]];
                    glm::vec3 v1 = vertList[triList[m+1]];
                    glm::vec3 v2 = vertList[triList[m+2]];
                    
                    glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
                    
                    Edge e0 = edgeSave[triList[m  ]]; 
                    Edge e1 = edgeSave[triList[m+1]]; 
                    Edge e2 = edgeSave[triList[m+2]]; 

                    local_map_i[e0].second += normal;
                    local_map_i[e1].second += normal;
                    local_map_i[e2].second += normal;

                    tris.push_back(e0);
                    tris.push_back(e1);
                    tris.push_back(e2);
                }
            }
        }
    }
}

void CubeMarch::MarchingCubes() {
    int total = nx - 1;
    int chunk = (total + num_threads - 1) / num_threads;

    std::vector<std::thread> threads;
    std::vector<std::vector<Edge>> local_triangles(num_threads, std::vector<Edge>{});
    std::vector<std::unordered_map<Edge, std::pair<glm::vec3, glm::vec3>, EdgeHash>> local_maps(num_threads, std::unordered_map<Edge, std::pair<glm::vec3, glm::vec3>, EdgeHash>{});

    for(int i = 0; i < num_threads; i++) {
        int begin = i * chunk;
        int end = std::min((i + 1) * chunk, total);

        if(begin >= total) { break; }

        threads.emplace_back(
            [this, begin, end, i, &local_triangles, &local_maps]() {
                march_cubes(begin, end, local_maps[i], local_triangles[i]);
            }
        );
    }

    for(auto& t: threads) { t.join(); }
    std::unordered_map<Edge, std::pair<glm::vec3, glm::vec3>, EdgeHash> global_map {};
    for(auto& local_map_i: local_maps) {
        for(auto& m: local_map_i) {
            Edge e = m.first;
            auto f = global_map.find(e);
            if(f != global_map.end()) {
                f->second.second += m.second.second;
            } else {
                global_map.insert({e, m.second});
            }
        }
    }

    triangles.clear();
    for(auto& tris: local_triangles) {
        // triangles.insert(triangles.end(), tris.begin(), tris.end());
        // triangles.insert()
        for(auto& e: tris){
            // triangles.push_back()
            auto& e_output = global_map[e];
            triangles.push_back(e_output.first);
            triangles.push_back(glm::normalize(e_output.second));
        }
    }
}
