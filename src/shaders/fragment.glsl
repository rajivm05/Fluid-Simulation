#version 330 core

in vec4 vColor;
out vec4 FragColor;

void main() {
    FragColor = vColor;

    vec2 c = gl_PointCoord * 2.0 - 1.0;
    float d = dot(c, c);
    if(d > 1.0) discard;

    FragColor.rgb *= (1.0 - d);
}
