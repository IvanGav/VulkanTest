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

const float OUTLINE_SCALE = 0.005f;

void main() {
    mat4 modelMat = pc.i.i[gl_InstanceIndex].model;
    vec4 worldPosition = modelMat * vec4(inPosition, 1.0);
    vec4 screenPosition = pc.u.u.proj * pc.u.u.view * worldPosition;
    screenPosition += OUTLINE_SCALE * normalize(pc.u.u.proj * pc.u.u.view * modelMat * vec4(inNormal, 0.0f)) * screenPosition.w;
    gl_Position = screenPosition;
}