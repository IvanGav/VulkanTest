#version 450
#extension GL_EXT_buffer_reference : require

struct UniformData {
    mat4 proj;
    mat4 view;
    vec3 camPosition;
    vec3 camLookDir;
    bool camOrtho;
};

struct InstanceData {
    mat4 model;
    uint texture;
};

layout(buffer_reference, std430, buffer_reference_align = 16) buffer UniformBuffer {
    UniformData u;
};

layout(buffer_reference, std430, buffer_reference_align = 16) buffer InstanceBuffer {
    InstanceData i[];
};

layout(push_constant) uniform PushConstants {
    UniformBuffer u;
    InstanceBuffer i;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec3 outNormal;
layout(location = 3) flat out uint textureIndex;

void main() {
    mat4 modelMat = pc.i.i[gl_InstanceIndex].model;
    vec4 worldPosition = modelMat * vec4(inPosition, 1.0);
    gl_Position = pc.u.u.proj * pc.u.u.view * worldPosition;
    outPosition = worldPosition.xyz;
    outUV = inUV;
    outNormal = (modelMat * vec4(inNormal, 0.0)).xyz;
    textureIndex = pc.i.i[gl_InstanceIndex].texture;
}