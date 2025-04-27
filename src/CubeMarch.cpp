#include "CubeMarch.h"

CubeMarch::CubeMarch(float lim_x, float lim_y, float lim_z, float len, float smoothing_dist, SPH* sph_ptr, float iv):
                                                    len_cube(len),
                                                    nx(2 * lim_x / len + 1),
                                                    ny(2 * lim_y / len + 1),
                                                    nz(2 * lim_z / len + 1), 
                                                    // cells(std::vector(nx, std::vector(ny, std::vector(nz, CubeCell {})))),
                                                    cells(nx * ny * nz, CubeCell {}),
                                                    num_threads(std::thread::hardware_concurrency()),
                                                    h(smoothing_dist),
                                                    sph(sph_ptr),
                                                    iso_value(iv) {

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

int CubeMarch::cube_index(int i, int j, int k) {
    return i * ny * nz + j * nz + k;
}

glm::vec3 CubeMarch::vertex_interpolation(float iso_value, int p1, int p2) {
    if(fabs(cells[p1].color - cells[p2].color < 1e-6)) return cells[p1].position;
    float mu = (iso_value - cells[p1].color )/ (cells[p2].color - cells[p1].color);
    return cells[p1].position + mu*(cells[p2].position - cells[p1].position);
}

void CubeMarch::MarchingCubes(){
    triangles.clear();
    
    for(int i = 0; i<nx-1; i++){
        for(int j = 0; j<ny-1; j++){
            for(int k = 0; k<nz - 1; k++){
                int cute_indices[8];
                for(int i =0; i<8; i++){
                    int dx = i        & 1;
                    int dy = (i >> 1) & 1;
                    int dz = (i >> 2) & 1;
                    cute_indices[i] = cube_index(i+dx, j+dy, k+dz);
                }

                int table_index = 0;
                for(int i = 0; i<8; i++){
                    // if()
                    if(cells[cute_indices[i]].color > iso_value) table_index |= (1<<i);
                }
                if(edgeTable[table_index] == 0) continue;
                glm::vec3 vertList[12];

                if (edgeTable[table_index] & 1)
                    vertList[0] = vertex_interpolation(iso_value, cute_indices[0], cute_indices[1]);
                if (edgeTable[table_index] & 2)
                    vertList[1] = vertex_interpolation(iso_value, cute_indices[1], cute_indices[2]);
                if (edgeTable[table_index] & 4)
                    vertList[2] = vertex_interpolation(iso_value, cute_indices[2], cute_indices[3]);
                if (edgeTable[table_index] & 8)
                    vertList[3] = vertex_interpolation(iso_value, cute_indices[3], cute_indices[0]);
                if (edgeTable[table_index] & 16)
                    vertList[4] = vertex_interpolation(iso_value, cute_indices[4], cute_indices[5]);
                if (edgeTable[table_index] & 32)
                    vertList[5] = vertex_interpolation(iso_value, cute_indices[5], cute_indices[6]);
                if (edgeTable[table_index] & 64)
                    vertList[6] = vertex_interpolation(iso_value, cute_indices[6], cute_indices[7]);
                if (edgeTable[table_index] & 128)
                    vertList[7] = vertex_interpolation(iso_value, cute_indices[7], cute_indices[4]);
                if (edgeTable[table_index] & 256)
                    vertList[8] = vertex_interpolation(iso_value, cute_indices[0], cute_indices[4]);
                if (edgeTable[table_index] & 512)
                    vertList[9] = vertex_interpolation(iso_value, cute_indices[1], cute_indices[5]);
                if (edgeTable[table_index] & 1024)
                    vertList[10] = vertex_interpolation(iso_value, cute_indices[2], cute_indices[6]);
                if (edgeTable[table_index] & 2048)
                    vertList[11] = vertex_interpolation(iso_value, cute_indices[3], cute_indices[7]);

                for(int i = 0; triTable[table_index][i] != -1; i+=3){
                    glm::vec3 v0 = vertList[triTable[table_index][i]];
                    glm::vec3 v1 = vertList[triTable[table_index][i+1]];
                    glm::vec3 v2 = vertList[triTable[table_index][i+1]];
                    // triangles.push_back(Triangle{v0, v1, v2});
                    triangles.push_back(v0);
                    triangles.push_back(v1);
                    triangles.push_back(v2);

                }
            }
        }
    }
    // return triangles;

}



