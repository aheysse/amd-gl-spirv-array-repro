#version 460 core

layout(binding = 0) uniform sampler2DArray arrayTexture;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

void main() {
    color = texture(arrayTexture, vec3(uv, 1.0));
}
