#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

layout(buffer_reference, scalar) buffer ShaderData {
    mat4 proj;
    mat4 view;
    mat4 model;
};

layout(push_constant) uniform PushConstants {
    ShaderData mats;
} data;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUVCoord;

layout(location = 0) out vec2 fragUVCoord;

void main() {
    gl_Position = data.mats.proj * data.mats.view * data.mats.model * vec4(inPosition, 1.0);
    fragUVCoord = inUVCoord;
}