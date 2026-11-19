#version 460 core

layout(binding = 0) uniform sampler2D plainTexture;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

void main() {
    color = texture(plainTexture, uv);
}
