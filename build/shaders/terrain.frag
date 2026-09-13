#version 460 core

in float height;

out vec3 finalcolor;

void main() {
    finalcolor = vec3(height / 2 + 1);
}
