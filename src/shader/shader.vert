#version 450
#extension GL_EXT_buffer_reference : require

layout(buffer_reference) buffer UniformData {
    mat4 proj;
    mat4 view;
    mat4 model;
};

layout(push_constant) uniform PushConstants {
    UniformData u;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUVCoord;

layout(location = 0) out vec2 fragUVCoord;

void main() {
    gl_Position = pc.u.proj * pc.u.view * pc.u.model * vec4(inPosition, 1.0);
    fragUVCoord = inUVCoord;
}