#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in float color;

out vec4 vColor;

uniform mat4 view;
uniform mat4 projection;
uniform vec2 screen_size;
uniform float sprite_size;
uniform float iso_value;

void main() {
    vec4 view_space = view * vec4(position, 1.0);
    vec4 project_sprite = projection * vec4(sprite_size, sprite_size, view_space.z, view_space.w);
    vec2 screen_sprite = 0.5 * screen_size * (project_sprite.xy / project_sprite.w);

    gl_PointSize = 0.5 * (screen_sprite.x + screen_sprite.y);
    gl_Position = projection * view_space;

    if(color > iso_value) {
        vColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
    } else {
        vColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
    }
}
