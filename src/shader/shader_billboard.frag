#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference : require

struct UniformData {
    mat4 proj;
    mat4 view;
    vec3 camPosition;
    vec3 camLookDir;
    bool camOrtho;
};

struct InstanceData {
    vec3 pos;
    float size;
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

layout(binding = 0) uniform sampler2D textures[];

layout(location = 0) in vec2 inUV;
layout(location = 1) flat in uint textureIndex;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 textureColor = texture(textures[textureIndex], inUV);
    if(textureColor.a < 0.5) { discard; }
    outColor = textureColor;
}