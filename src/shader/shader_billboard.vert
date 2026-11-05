#version 450
#extension GL_EXT_buffer_reference : require

/*
struct InstanceData {
    mat4 model;
    uint texture;
};
*/

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

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec2 outUV;
layout(location = 1) flat out uint textureIndex;

vec3 quad[6] = {
    vec3( 0.5,  0.5, 0.0), // 0
    vec3( 0.5, -0.5, 0.0), // 1
    vec3(-0.5, -0.5, 0.0), // 2
    vec3(-0.5, -0.5, 0.0), // 2
    vec3(-0.5,  0.5, 0.0), // 3
    vec3( 0.5,  0.5, 0.0), // 0
};

void main() {
    float scale = pc.i.i[gl_InstanceIndex].size;
    vec3 pos = pc.i.i[gl_InstanceIndex].pos;
    vec3 off = quad[gl_VertexIndex];
    
    vec3 worldUp = vec3(0.0, 0.0, 1.0);
    vec3 camRight = normalize(cross(worldUp, pc.u.u.camLookDir));
    vec3 camUp = cross(camRight, pc.u.u.camLookDir);

    vec3 worldPosition = pos + (camRight * off.x * scale) + (camUp * off.y * scale);

    gl_Position = pc.u.u.proj * pc.u.u.view * vec4(worldPosition, 1.0);
    outUV = off.xy + 0.5;
    textureIndex = pc.i.i[gl_InstanceIndex].texture;
}





























    /*
    // mat4 modelMat = pc.i.i[gl_InstanceIndex].model;
    mat4 modelMat = translate(mat4(0.5), quad[gl_VertexIndex]);
    vec4 worldPosition = modelMat * vec4(quad[gl_VertexIndex] * scale, 0.5);
    gl_Position = pc.u.u.proj * pc.u.u.view * worldPosition;
    outUV = inUV;
    textureIndex = pc.i.i[gl_InstanceIndex].texture;
    */