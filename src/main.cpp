#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <random>
#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"

#include "SpatialHash.h"
#include "shader.h"
#include "Particle.h"




std::vector<Particle> CreateSphereParticles(int count, float radius) {
    std::vector<Particle> particles;
    // particles.reserve(count);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> unitDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> colorDist(0.0f, 1.0f);

    for(int i = 0; i < count; ++i) {
        // Generate random point on sphere surface
        // float theta = 2.0f * glm::pi<float>() * dist(gen);
        // float phi = acos(2.0f * dist(gen) - 1.0f);
        float theta = 2.0f * glm::pi<float>() * unitDist(gen);
        float phi   = acos(1.0f - 2.0f * unitDist(gen));
    
        
        Particle p;
        p.position = glm::vec3(
            radius * sin(phi) * cos(theta),
            radius * sin(phi) * sin(theta),
            // radius * cos(phi)
            0.0f
        );
        
        // Random velocity (scaled for stability)
        p.velocity = glm::vec3(
            // 0.0f, 0.0f, 0.0f
            dist(gen) * 0.001f,
            dist(gen) * 0.001f,
            // dist(gen) * 0.001f
            0.0f
        );
        
        // Random pastel color
        p.color = glm::vec4(
            // 0.5f + colorDist(gen) * 0.5f,
            // 0.5f + colorDist(gen) * 0.5f,
            // 0.5f + colorDist(gen) * 0.5f,
            62.0f/255,
            164.0f/255,
            240.0f/255,
            0.8f
        );
        
        particles.push_back(p);
    }
    
    return particles;
}

void netAcceleration(std::vector<Particle>& particles){
    glm::vec3 gravity = {0.0f, -0.0098f, 0.0f};
    for(auto& p: particles){
        p.acceleration = gravity;
    }
}

void computeForces(Particle& p) {
    // glm::vec2 f_pressure(0.0f);
    // glm::vec2 f_viscosity(0.0f);
    
    // for(auto* neighbor : p.neighbors) {
    //     glm::vec2 r = p.position - neighbor->position;
        
    //     // Pressure force
    //     f_pressure += (p.pressure + neighbor->pressure) / (2 * neighbor->density) 
    //                 * W_spiky_grad(r, h) * mass;
        
    //     // Viscosity force
    //     f_viscosity += (neighbor->velocity - p.velocity) / neighbor->density 
    //                 * (45.0f / (M_PI * pow(h,6))) * (h - glm::length(r)) * mass;
    // }
    
    // p.acceleration = (f_pressure + mu*f_viscosity)/p.density + glm::vec2(0.0f, -9.8f);
    // p.acceleration = glm::vec3(0.0f, -0.0098f, 0.0);
    for(auto* neighbor : p.neighbors) {
        // p.color = glm::vec4(0.0, 0.0, 0.0, 0.0);
        break;
    }

    
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

    float sphereRadius = 1.0f;
    std::vector<Particle> particles = CreateSphereParticles(1000, sphereRadius);
    // std::vector<Particle> particles = StreamFlow(100, 0.6f);


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

    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    std::cout << vendor << "\t" << renderer << std::endl;


    Shader shader {"../src/shaders/vertex.glsl", "../src/shaders/fragment.glsl"};

    // Main loop
    float deltaTime = 0.016;
    float damping_factor = 0.8f;

    // In main() or wherever you create SpatialHash:
    float h = 0.06f;  // Your smoothing length
    SpatialHash spatialHash(2.0f*h, h);  // cellSize=2h, h=smoothingLength


    while (!glfwWindowShouldClose(window)) {
        spatialHash.update(particles);
        for(auto& p : particles) spatialHash.findNeighbors(p);
        netAcceleration(particles);
        for(auto& p : particles) computeForces(p);


        for (auto& p : particles) {
            p.velocity += p.acceleration * deltaTime;
            p.position += p.velocity;


            // Bounce on X edges
            if (p.position.x < -0.95f) 
            {
                p.position.x = -0.95f;
                p.velocity.x *= -p.velocity.x * damping_factor;
            }
            if (p.position.x > 0.95f) 
            {
                p.position.x = 0.95f;
                p.velocity.x *= -p.velocity.x * damping_factor;
            }
            // // Bounce on Y edges
            if (p.position.y < -0.95f) 
                {
                    p.position.y = -0.95f;
                    p.velocity.y = -p.velocity.y * damping_factor;
                }
            if (p.position.y > 0.95f) 
            {
                p.position.y = 0.95f;
                p.velocity.y = -p.velocity.y * damping_factor;
            }
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


