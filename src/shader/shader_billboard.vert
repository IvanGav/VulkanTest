#version 450
#extension GL_EXT_buffer_reference : require

struct UniformData {
    mat4 proj;
    mat4 view;
    vec3 camPosition;
};

struct InstanceData {
    mat4 model;
    uint texture;
};

layout(buffer_reference, std430, buffer_reference_align = 16) buffer ShaderData {
    UniformData u;
    InstanceData i[];
};

layout(push_constant) uniform PushConstants {
    ShaderData data;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec2 outUV;
layout(location = 1) flat out uint textureIndex;

void main() {
    mat4 modelMat = pc.data.i[gl_InstanceIndex].model;
    vec4 worldPosition = modelMat * vec4(inPosition, 1.0);
    gl_Position = pc.data.u.proj * pc.data.u.view * worldPosition;
    outUV = inUV;
    textureIndex = pc.data.i[gl_InstanceIndex].texture;
}