#version 410 core
#pragma debug(on)
#pragma optimize(off)

layout (location = 0) out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 1.0, 1.0, 0.2);
}