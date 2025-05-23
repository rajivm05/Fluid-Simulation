#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;   // Position in world space
out vec3 Normal;    // Normal in world space

// uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // FragPos = vec3(model * vec4(aPos, 1.0));
    // Normal = mat3(transpose(inverse(model))) * aNormal;

    FragPos = aPos;
    Normal = aNormal;

    gl_Position = projection * view * vec4(aPos, 1.0);
}