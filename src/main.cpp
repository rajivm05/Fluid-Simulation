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

int main() {
    const int width = 800;
    const int height = 600;
    const char* window_name = "Particles Array";

    GLFWwindow* window = gl_init(width, height, window_name);

    float delta_time = 0.016f;
    // float delta_time = 0.0083f;
    float damping_factor = 0.4;
    int particle_count = 1;
    float lim_x = 0.2f;  
    float lim_y = 0.2f;
    float lim_z = 0.2f;
    glm::vec4 box_color = glm::vec4(0.0, 0.0, 0.0, 0.2);
    SPH sph {delta_time, damping_factor, particle_count, lim_x, lim_y, lim_z, box_color};

    // sph.initialize_particles(glm::vec3(0.0f, 0.0f, 0.0f), 0.5);
    sph.initialize_particles_cube(glm::vec3(0.0f, 0.0f, 0.0f), 0.5/5, 0.05/5);
    
    sph.create_cuboid();

    // VAO and VBO setup
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

    glm::vec3 cam_pos(5.0f/5, 2.0f/5, 5.0f/5);
    glm::vec3 cam_target(0.0f, 0.0f, 0.0f);
    glm::vec3 cam_up(0.0f, 1.0f, 0.0f);
    float cam_fov(glm::radians(45.0f));
    float cam_near = 0.1f;
    float cam_far = 100.0f;

    Camera cam {cam_pos, cam_target, cam_up, cam_fov, (float) width, (float) height, cam_near, cam_far};

    const float sprite_size = 0.2/3;

    while (!glfwWindowShouldClose(window)) {
        spatialHash.build(sph.particles);

        #pragma omp parallel for
        for(auto& p: sph.particles) 
        { 
            spatialHash.queryNeighbors(p, 2*h);

        }

        sph.calculate_forces();
        sph.update_state();
        sph.boundary_conditions(sprite_size/6);
        for(auto& p: sph.particles){
            p.hash_value = spatialHash.computeHash(spatialHash.positionToCell(p.position));
        }
        

        // Update buffer
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sph.particles.size() * sizeof(Particle), sph.particles.data());

        glBindBuffer(GL_ARRAY_BUFFER, cVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sph.box_positions.size() * sizeof(glm::vec3), sph.box_positions.data());

        // Draw
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        shader.setMatrix("view", cam.view);
        shader.setMatrix("projection", cam.projection);
        shader.setVec2("screen_size", glm::vec2(width, height));
        shader.setFloat("sprite_size", sprite_size);
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, sph.particles.size());

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
