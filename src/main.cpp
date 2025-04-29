#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "SpatialHash.h"
#include "shader.h"
#include "particle.h"
#include "camera.h"
#include "sph.h"
#include "frame.h"
#include "CubeMarch.h"
#include "sph_consts.h"

#include <thread>
#include <chrono>

#include <fstream>
#include <string>
#include <iomanip>
#include <sstream>

using namespace main_c;

enum class RenderMode {
    render,
    save,
    load
};

std::tuple<FrameHeader, std::vector<Particle_buffer> , std::vector<glm::vec3>>
load_frame_data(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) throw std::runtime_error("Can't open " + filename);

    // Read header
    FrameHeader header;
    in.read(reinterpret_cast<char*>(&header), sizeof(FrameHeader));
    
    // Validate
    if (std::string(header.magic, 3) != "SPH") 
        throw std::runtime_error("Invalid file format");
    if (header.version != 3)
        throw std::runtime_error("Unsupported version");

    // Read particles
    std::vector<Particle_buffer> particles(header.particle_count);
    in.read(reinterpret_cast<char*>(particles.data()), 
           header.particle_count * sizeof(Particle_buffer));

    std::vector<glm::vec3> triangles(header.triangle_count);
    in.read(reinterpret_cast<char*>(triangles.data()), header.triangle_count * sizeof(glm::vec3));

    return {header, particles, triangles};
}

void save_frame_data(SPH& sph, std::unique_ptr<CubeMarch>& cm, int frame_number, const Camera& cam, 
    const std::string& prefix = "../frames/frame_") {
    std::ostringstream filename;
    filename << prefix << std::setw(4) << std::setfill('0') << frame_number << ".bin";

    std::ofstream out(filename.str(), std::ios::binary);
    if (!out) {
    std::cerr << "Error opening: " << filename.str() << std::endl;
    return;
    }

    // Write header
    FrameHeader header;
    header.timestamp = frame_number * sph.delta_time;
    header.particle_count = static_cast<uint32_t>(sph.particles.size());
    header.h = sph.h;
    header.dt = sph.delta_time;
    header.view = cam.view;
    header.projection = cam.projection;
    header.gravity = sph.gravity;
    header.damping_factor = sph.damping_factor;
    header.box_limits = glm::vec4(sph.lim_x, sph.lim_y, sph.lim_z, 5);
    header.cube_len = cm->len_cube;
    header.iso_value = cm->iso_value;
    header.triangle_count = cm->triangles.size();

    out.write(reinterpret_cast<char*>(&header), sizeof(FrameHeader));

    // Write particles
    for (const auto& p : sph.particles) {
        Particle_buffer fp;
        fp.position = p.position;
        fp.density = p.density;
        fp.velocity = p.velocity;
        fp.pressure = p.pressure;
        fp.color = p.color;
        out.write(reinterpret_cast<char*>(&fp), sizeof(Particle_buffer));
    }

    out.write(reinterpret_cast<const char*>(cm->triangles.data()), cm->triangles.size() * sizeof(glm::vec3));

    out.close();
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    main_c::width = width;
    main_c::height = width;
    glViewport(0, 0, width, height);
}

GLFWwindow* gl_init(const int width, const int height, const char* window_name) {
    if(!glfwInit()) { exit(1); }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

    GLFWwindow* window = glfwCreateWindow(width, height, window_name, nullptr, nullptr);
    if(!window) {
        glfwTerminate();
        exit(1);
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    if(!gladLoadGL(glfwGetProcAddress)) { exit(0); }
    
    glViewport(0, 0, width, height);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return window;
}

int main(int argc, char* argv[]) {
    if(argc < 4) {
        std::cerr << "Usage: ./simulator [render|save|load] [true|false] [true|false]" << std::endl;
        return 1;
    }

    std::string mode_s = argv[1];
    std::string march_s = argv[2];
    std::string phong_s = argv[3];

    RenderMode mode;
    if(mode_s == "render") { mode = RenderMode::render; }
    else if(mode_s == "save") { mode = RenderMode::save; }
    else if(mode_s == "load") { mode = RenderMode::load; }
    else {
        std::cerr << "Usage: ./simulator [render|save|load]" << std::endl;
        return 1;
    }
    std::cout << mode_s << " mode" << std::endl;

    bool turnOnMarchingCubes;
    if(march_s == "true") {
        turnOnMarchingCubes = true;
    } else {
        turnOnMarchingCubes = false;
    }

    bool turnOnPhongShading = false;
    if(phong_s == "true") {
        turnOnPhongShading = true;
    }

    int numThreads = std::thread::hardware_concurrency();
    std::cout << "Using " << numThreads << " threads\n";

    GLFWwindow* window = gl_init(width, height, window_name);

    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    std::cout << vendor << "\t" << renderer << std::endl;
    
    Shader shader {"../src/shaders/vertex.glsl", "../src/shaders/fragment.glsl"};
    Shader cShader {"../src/shaders/cVertex.glsl", "../src/shaders/cFragment.glsl"};
    Shader mShader {"../src/shaders/mVertex.glsl", "../src/shaders/fragment.glsl"};
    Shader phongShader {"../src/shaders/phongvert.glsl", "../src/shaders/phongfrag.glsl"};

    Camera cam {cam_pos, cam_target, cam_up, cam_fov, (float) width, (float) height, cam_near, cam_far};
    SpatialHash spatialHash(h);
    SPH sph {h, lim_x, lim_y, lim_z, sprite_size, spatialHash};
    std::unique_ptr<CubeMarch> cm = nullptr;

    // sph.initialize_particles_sphere(sphere_count, sphere_center, sphere_radius);
    sph.initialize_particles_cube(cube_center, cube_side_length, cube_spacing);
    
    sph.create_cuboid();

    // SPH Particles
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sph.particles.size() * sizeof(Particle), sph.particles.data(), GL_DYNAMIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, position));
    // Color attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, color));

    // Container
    GLuint cVAO, cVBO;
    glGenVertexArrays(1, &cVAO);
    glGenBuffers(1, &cVBO);
    glBindVertexArray(cVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cVBO);
    glBufferData(GL_ARRAY_BUFFER, sph.box_positions.size() * sizeof(glm::vec3), sph.box_positions.data(), GL_DYNAMIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    // Mesh
    GLuint tVAO, tVBO;
    glGenVertexArrays(1, &tVAO);
    glGenBuffers(1, &tVBO);

    // Cube March grid particles
    GLuint mVAO, mVBO;
    glGenVertexArrays(1, &mVAO);
    glGenBuffers(1, &mVBO);

    if(turnOnMarchingCubes) {
        cm.reset(new CubeMarch{2*lim_x, 2*lim_y, 2*lim_z, len_cube, cm_h, &sph, iso_value, spatialHash});
        int max_triangles = 5 * cm->cells.size();

        glBindVertexArray(tVAO);
        glBindBuffer(GL_ARRAY_BUFFER, tVBO);
        glBufferData(GL_ARRAY_BUFFER, max_triangles * 6 * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

        // Position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 2*sizeof(glm::vec3), (void*)0);

        // Normal attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 2*sizeof(glm::vec3), (void*) (sizeof(glm::vec3)));

        // glBindVertexArray(mVAO);
        // glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        // glBufferData(GL_ARRAY_BUFFER, cm->cells.size() * sizeof(CubeCell), cm->cells.data(), GL_DYNAMIC_DRAW);

        // // Position attribute
        // glEnableVertexAttribArray(0);
        // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CubeCell), (void*)offsetof(CubeCell, position));
        // // Color attribute
        // glEnableVertexAttribArray(1);
        // glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(CubeCell), (void*)offsetof(CubeCell, color));
    }
    
    float radius = 5.0f;  // distance from center
    int frame_number = 0;
    int max_frames = 1800;

    while (!glfwWindowShouldClose(window)) {
    // while(max_frames-- >= 0){
        // std::cout << max_frames <<std::endl;
        if(mode == RenderMode::render || mode == RenderMode::save) {
            sph.parallel(&SPH::update_hash);
            spatialHash.build(sph.particles);

            // float angle = glfwGetTime()/2.0f;
            // cam_pos = glm::vec3(
            //     radius * std::sin(angle),  // X
            //     2.0f,                      // Y (height stays fixed)
            //     radius * std::cos(angle)   // Z
            // )/5.0f;
            // cam.view = glm::lookAt(cam_pos, cam_target, cam_up);
    
            sph.parallel(&SPH::update_neighbors);
            if(turnOnMarchingCubes) { cm->parallel(&CubeMarch::update_neighbors); }

            sph.parallel(&SPH::update_properties);
            sph.parallel(&SPH::calculate_forces);
            sph.parallel(&SPH::update_state);
            sph.parallel(&SPH::boundary_conditions);
            
            if(turnOnMarchingCubes) { cm->parallel(&CubeMarch::update_color); }
        }

        if(mode == RenderMode::load){
            try {
                std::ostringstream filename;
                std::cout << frame_number <<std::endl;
                filename << "../frames/frame_" << std::setw(4) << std::setfill('0') << frame_number++ << ".bin";
                std::cout << filename.str() <<std::endl;
                auto [header, buffer, triangles] = load_frame_data(filename.str());
                if(turnOnMarchingCubes) { cm->load_triangles(triangles); }

                // Update buffer
                glBindBuffer(GL_ARRAY_BUFFER, VBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, buffer.size() * sizeof(Particle), buffer.data());

                glBindBuffer(GL_ARRAY_BUFFER, cVBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sph.box_positions.size() * sizeof(glm::vec3), sph.box_positions.data());
                std::this_thread::sleep_for(std::chrono::duration<float>(1.0f / 30.0f));
    
            } catch (const std::exception& e) {
                std::cerr << "Loading failed: " << e.what() << std::endl;
            }
        }
        else if(mode == RenderMode::render){
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sph.particles.size() * sizeof(Particle), sph.particles.data());

            glBindBuffer(GL_ARRAY_BUFFER, cVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sph.box_positions.size() * sizeof(glm::vec3), sph.box_positions.data());

            if(turnOnMarchingCubes) { cm->MarchingCubes(); }

        }
        else if(mode == RenderMode::save){
            cm->MarchingCubes();
            save_frame_data(sph, cm, frame_number++, cam);
            continue;
        }      

        //render cube march stuff  
        if(turnOnMarchingCubes) {
            glBindVertexArray(tVAO);
            glBindBuffer(GL_ARRAY_BUFFER, tVBO);
            glBufferData(GL_ARRAY_BUFFER, cm->triangles.size() * sizeof(glm::vec3), cm->triangles.data(), GL_DYNAMIC_DRAW);
            // glBufferSubData(GL_ARRAY_BUFFER, 0, cm->triangles.size() * sizeof(glm::vec3), cm->triangles.data());
            
            // Position attribute
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 2*sizeof(glm::vec3), (void*)0);

            // Normal attribute
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 2*sizeof(glm::vec3), (void*) (sizeof(glm::vec3)));

            // glBindBuffer(GL_ARRAY_BUFFER, mVBO);
            // glBufferSubData(GL_ARRAY_BUFFER, 0, cm->cells.size() * sizeof(CubeCell), cm->cells.data());
        }

        // Draw
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if(!turnOnMarchingCubes) {
            shader.use();
            shader.setMatrix("view", cam.view);
            shader.setMatrix("projection", cam.projection);
            shader.setVec2("screen_size", glm::vec2(width, height));
            shader.setFloat("sprite_size", sprite_size);
            glBindVertexArray(VAO);
            glDrawArrays(GL_POINTS, 0, sph.particles.size());
        }

        if(turnOnMarchingCubes) {
            if(!turnOnPhongShading) {
                cShader.use();
                cShader.setMatrix("view", cam.view);
                cShader.setMatrix("projection", cam.projection);
                cShader.setVec4("color", glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
                glBindVertexArray(tVAO);
                glDrawArrays(GL_TRIANGLES, 0, cm->triangles.size());
            } else {
                phongShader.use();
                phongShader.setMatrix("view", cam.view);
                phongShader.setMatrix("projection", cam.projection);
                phongShader.setVec3("lightPos", cam_pos);
                phongShader.setVec3("viewPos", cam_pos);
                phongShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
                phongShader.setVec3("objectColor", glm::vec3(0.0f, 0.0f, 1.0f));
                glBindVertexArray(tVAO);
                glDrawArrays(GL_TRIANGLES, 0, cm->triangles.size());
            }
        }

        // mShader.use();
        // mShader.setMatrix("view", cam.view);
        // mShader.setMatrix("projection", cam.projection);
        // mShader.setVec2("screen_size", glm::vec2(width, height));
        // mShader.setFloat("sprite_size", 0.01);
        // mShader.setFloat("iso_value", iso_value);
        // glBindVertexArray(mVAO);
        // glDrawArrays(GL_POINTS, 0, cm->cells.size());

        // if(!turnOnMarchingCubes) {
        cShader.use();
        cShader.setMatrix("view", cam.view);
        cShader.setMatrix("projection", cam.projection);
        cShader.setVec4("color", sph.box_color);
        glBindVertexArray(cVAO);
        glDrawArrays(GL_TRIANGLES, 0, sph.box_positions.size());  
        // }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &cVAO);
    glDeleteVertexArrays(1, &tVAO);
    glDeleteVertexArrays(1, &mVAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &cVBO);
    glDeleteBuffers(1, &tVBO);
    glDeleteBuffers(1, &mVBO);
    glDeleteProgram(shader.ID);

    glfwTerminate();
    return 0;
}
