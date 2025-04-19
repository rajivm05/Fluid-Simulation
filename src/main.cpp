#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <random>
#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"

#include "shader.h"


// Particle struct with position, velocity, and color
struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;
};

std::vector<Particle> CreateSphereParticles(int count, float radius) {
    std::vector<Particle> particles;
    particles.reserve(count);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> colorDist(0.0f, 1.0f);

    for(int i = 0; i < count; ++i) {
        // Generate random point on sphere surface
        float theta = 2.0f * glm::pi<float>() * dist(gen);
        float phi = acos(2.0f * dist(gen) - 1.0f);
        
        Particle p;
        p.position = glm::vec3(
            radius * sin(phi) * cos(theta),
            radius * sin(phi) * sin(theta),
            radius * cos(phi)
        );
        
        // Random velocity (scaled for stability)
        p.velocity = glm::vec3(
            dist(gen) * 0.001f,
            dist(gen) * 0.001f,
            dist(gen) * 0.001f
        );
        
        // Random pastel color
        p.color = glm::vec4(
            0.5f + colorDist(gen) * 0.5f,
            0.5f + colorDist(gen) * 0.5f,
            0.5f + colorDist(gen) * 0.5f,
            0.8f
        );
        
        particles.push_back(p);
    }
    
    return particles;
}


int main() {
    
    // GLFW and OpenGL context setup

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Particles Array", nullptr, nullptr);
    
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    
    if (!gladLoadGL(glfwGetProcAddress)) return -1;
    
    glViewport(0, 0, 800, 600);
    glEnable(GL_PROGRAM_POINT_SIZE);

    std::vector<Particle> particles = CreateSphereParticles(1000, 0.6f);


    // VAO and VBO setup
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(Particle), particles.data(), GL_DYNAMIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, position));
    // Color attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, color));

    Shader shader {"../src/shaders/vertex.glsl", "../src/shaders/fragment.glsl"};

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        for (auto& p : particles) {
            p.position += p.velocity;
            // Bounce on X edges
            if (p.position.x > 0.95f || p.position.x < -0.95f) p.velocity.x *= -1.0f;
            // Bounce on Y edges
            if (p.position.y > 0.95f || p.position.y < -0.95f) p.velocity.y *= -1.0f;
        }
        // Update buffer
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, particles.size() * sizeof(Particle), particles.data());

        // Draw
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        shader.use();
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, particles.size());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shader.ID);

    glfwTerminate();
    return 0;
}
