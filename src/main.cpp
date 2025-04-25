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

#include <thread>
#include <chrono>

#include <fstream>
#include <string>
#include <iomanip>
#include <system_error>
#include <sstream>



void threadedQuery(SpatialHash& sh, std::vector<Particle>& particles, int start, int end, float h) {
    for (int i = start; i < end; ++i)
        sh.queryNeighbors(particles[i], 2 * h);
}

void parallelNeighborQuery(SPH& sph, SpatialHash& sh, float h, int numThreads = std::thread::hardware_concurrency()) {
    int N = sph.particles.size();
    int chunk = (N + numThreads - 1) / numThreads;

    std::vector<std::thread> threads;

    for (int t = 0; t < numThreads; ++t) {
        int start = t * chunk;
        int end = std::min(N, start + chunk);
        threads.emplace_back(threadedQuery, std::ref(sh), std::ref(sph.particles), start, end, h);
    }

    for (auto& t : threads)
        t.join();
}

std::vector<Particle_buffer> load_frame_data(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary | std::ios::ate);
    if (!in) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    std::streamsize file_size = in.tellg();
    in.seekg(0, std::ios::beg);

    if (file_size % sizeof(Particle_buffer) != 0) {
        throw std::runtime_error("File size does not match Particle struct size in " + filename);
    }

    std::cout << file_size << std::endl;

    size_t count = file_size / sizeof(Particle_buffer);
    std::cout << count << std::endl;
    std::vector<Particle_buffer> particles(count);

    if (!in.read((char*)(particles.data()), file_size)) {
        throw std::runtime_error("Failed to read data from: " + filename);
    }

    return particles;
}

void save_frame_data(const std::vector<Particle_buffer>& particles, int frame_number, const std::string& prefix = "../frames/frame_") {
    std::ostringstream filename;
    filename << prefix << std::setw(4) << std::setfill('0') << frame_number << ".bin";

    std::ofstream out(filename.str(), std::ios::binary);
    if (!out) {
        std::cerr << "Error: Could not open file for writing: " << filename.str() << std::endl;
        return;
    }

    // Write the entire vector's data in one call
    out.write(reinterpret_cast<const char*>(particles.data()), particles.size() * sizeof(Particle_buffer));
    out.close();
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
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

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return window;
}

int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cerr << "Usage: ./your_program [Render|Save|Load]" << std::endl;
        return 1;
    }

    std::string mode = argv[1];
    // renderMode modeEnum;
    // if(mode == "render"){
    //     modeEnum = renderMode::render;
    // }
    std::cout << mode << " mode" << std::endl;

    int numThreads = std::thread::hardware_concurrency();
    std::cout << "Using " << numThreads << " threads\n";
    const int width = 800;
    const int height = 600;
    const char* window_name = "Particles Array";

    GLFWwindow* window = gl_init(width, height, window_name);

    float delta_time = 0.016f;
    // float delta_time = 0.0083f;
    float damping_factor = 0.4;
    int particle_count = 1000;
    float scale = 5.0f;
    float lim_x = 1.0f/scale;  
    float lim_y = 1.0f/pow(scale, 1.5);
    float lim_z = 1.0f/scale;
    glm::vec4 box_color = glm::vec4(0.0, 0.0, 0.0, 0.2);
    SPH sph {delta_time, damping_factor, particle_count, lim_x, lim_y, lim_z, box_color};

    sph.initialize_particles(glm::vec3(0.0f, 0.0f, 0.0f), 0.5/scale);
    // sph.initialize_particles_cube(glm::vec3(0.0f, 0.0f, 0.0f), 0.5/scale, 0.05/(scale ));
    
    sph.create_cuboid();

    // VAO and VBO setup
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sph.buffer.size() * sizeof(Particle_buffer), sph.buffer.data(), GL_DYNAMIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle_buffer), (void*)offsetof(Particle_buffer, position));
    // Color attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Particle_buffer), (void*)offsetof(Particle_buffer, color));


    //cuboid conditions
    GLuint cVAO, cVBO;
    glGenVertexArrays(1, &cVAO);
    glGenBuffers(1, &cVBO);
    glBindVertexArray(cVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cVBO);
    glBufferData(GL_ARRAY_BUFFER, sph.box_positions.size() * sizeof(glm::vec3), sph.box_positions.data(), GL_DYNAMIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    std::cout << vendor << "\t" << renderer << std::endl;

    Shader shader {"../src/shaders/vertex.glsl", "../src/shaders/fragment.glsl"};
    Shader cShader {"../src/shaders/cVertex.glsl", "../src/shaders/cFragment.glsl"};

    float h = 0.06f;
    SpatialHash spatialHash(2.0f*h);
    for(auto& p: sph.particles){
        p.hash_value = spatialHash.computeHash(spatialHash.positionToCell(p.position));
    }

    glm::vec3 cam_pos(5.0f/scale, 2.0f/scale, 5.0f/scale);
    glm::vec3 cam_target(0.0f, 0.0f, 0.0f);
    glm::vec3 cam_up(0.0f, 1.0f, 0.0f);
    float cam_fov(glm::radians(45.0f));
    float cam_near = 0.1f;
    float cam_far = 100.0f;

    Camera cam {cam_pos, cam_target, cam_up, cam_fov, (float) width, (float) height, cam_near, cam_far};

    const float sprite_size = 0.2;

    float radius = 5.0f;  // distance from center
    int frame_number = 0;
    int max_frames = 1800;

    // while (!glfwWindowShouldClose(window)) {
    while(max_frames-- >= 0){
        std::cout << max_frames <<std::endl;
        if(mode == "render" || mode == "save"){
            spatialHash.build(sph.particles);

            // float angle = glfwGetTime()/2.0f;
            // cam_pos = glm::vec3(
            //     radius * std::sin(angle),  // X
            //     2.0f,                      // Y (height stays fixed)
            //     radius * std::cos(angle)   // Z
            // )/5.0f;
            // cam.view = glm::lookAt(cam_pos, cam_target, cam_up);
    
            parallelNeighborQuery(sph, spatialHash, h);
    
            sph.calculate_forces();
            sph.update_state(numThreads);
            sph.boundary_conditions(sprite_size/(scale + 1));
            for(auto& p: sph.particles){
                p.hash_value = spatialHash.computeHash(spatialHash.positionToCell(p.position));
            }
        }

        if(mode == "load"){
            try {
                std::ostringstream filename;
                std::cout << frame_number <<std::endl;
                filename << "../frames/frame_" << std::setw(4) << std::setfill('0') << frame_number++ << ".bin";
                std::cout << filename.str() <<std::endl;
                
                auto buffer = load_frame_data(filename.str());
                // std::cout << buffer[0].position.x <<std::endl;
                // Update buffer
                glBindBuffer(GL_ARRAY_BUFFER, VBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, buffer.size() * sizeof(Particle_buffer), buffer.data());

                glBindBuffer(GL_ARRAY_BUFFER, cVBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sph.box_positions.size() * sizeof(glm::vec3), sph.box_positions.data());
                std::this_thread::sleep_for(std::chrono::duration<float>(1.0f / 30.0f));
    
            } catch (const std::exception& e) {
                std::cerr << "Loading failed: " << e.what() << std::endl;
            }
        }
        else if(mode == "render"){
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sph.buffer.size() * sizeof(Particle_buffer), sph.buffer.data());

            glBindBuffer(GL_ARRAY_BUFFER, cVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sph.box_positions.size() * sizeof(glm::vec3), sph.box_positions.data());
        }
        else if(mode == "save"){
            save_frame_data(sph.buffer, frame_number++);
            // return 0;
            continue;
        }

        


        // Update buffer
        

        // Draw
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        shader.setMatrix("view", cam.view);
        shader.setMatrix("projection", cam.projection);
        shader.setVec2("screen_size", glm::vec2(width, height));
        shader.setFloat("sprite_size", sprite_size);
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, sph.buffer.size());

        cShader.use();
        cShader.setMatrix("view", cam.view);
        cShader.setMatrix("projection", cam.projection);
        cShader.setVec4("box_color", sph.box_color);
        glBindVertexArray(cVAO);
        glDrawArrays(GL_TRIANGLES, 0, sph.box_positions.size());  

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shader.ID);

    glfwTerminate();
    return 0;
}
