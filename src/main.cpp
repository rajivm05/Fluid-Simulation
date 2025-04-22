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
    
    if(!gladLoadGL(glfwGetProcAddress)) { exit(0); }
    
    glViewport(0, 0, width, height);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);

    return window;
}

int main() {
    const int width = 800;
    const int height = 600;
    const char* window_name = "Particles Array";

    GLFWwindow* window = gl_init(width, height, window_name);

    float delta_time = 0.016;
    float damping_factor = 0.8;
    int particle_count = 100;
    int lim_x = 5;
    int lim_y = 5;
    int lim_z = 5;
    SPH sph {delta_time, damping_factor, particle_count, lim_x, lim_y, lim_z};

    sph.initialize_particles(glm::vec3(0.0f, 0.0f, 0.0f), 2);

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

    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    std::cout << vendor << "\t" << renderer << std::endl;

    Shader shader {"../src/shaders/vertex.glsl", "../src/shaders/fragment.glsl"};

    float h = 0.06f;
    SpatialHash spatialHash(2.0f*h, h);

    glm::vec3 cam_pos(0.0f, 0.0f, 20.0f);
    glm::vec3 cam_target(0.0f, 0.0f, 0.0f);
    glm::vec3 cam_up(0.0f, 1.0f, 0.0f);
    float cam_fov(glm::radians(45.0f));
    float cam_near = 0.1f;
    float cam_far = 100.0f;

    Camera cam {cam_pos, cam_target, cam_up, cam_fov, (float) width, (float) height, cam_near, cam_far};

    while (!glfwWindowShouldClose(window)) {
        spatialHash.update(sph.particles);
        for(auto& p: sph.particles) { spatialHash.findNeighbors(p); }

        sph.calculate_forces();
        sph.update_state();
        sph.boundary_conditions();

        // Update buffer
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sph.particles.size() * sizeof(Particle), sph.particles.data());

        int transformLoc = glGetUniformLocation(shader.ID, "transform");
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(cam.transform));

        // Draw
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.use();
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, sph.particles.size());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shader.ID);

    glfwTerminate();
    return 0;
}
