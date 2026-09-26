#version 450
#extension GL_EXT_nonuniform_qualifier : require
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

layout(binding = 0) uniform sampler2D textures[];

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) flat in uint textureIndex;

layout(location = 0) out vec4 outColor;

const vec3 lightPos = vec3(-2.0, 2.0, 2.0);
const vec3 lightColor = vec3(1.0, 1.0, 1.0);
const float lightPower = 100.0;
const vec3 ambientColor = vec3(0.1, 0.0, 0.0);
const vec3 diffuseColor = vec3(0.5, 0.5, 0.5);
const vec3 specColor = vec3(1.0, 1.0, 1.0);
const float shininess = 16.0;
const float screenGamma = 2.2;

void main() {
    vec3 normal = normalize(inNormal);
    vec3 lightDir = lightPos - inPosition;
    float distance = dot(lightDir, lightDir);
    lightDir = normalize(lightDir);

    float lambertian = max(dot(lightDir, normal), 0.0);
    float specular = 0.0;

    if (lambertian > 0.0) {

        vec3 viewDir = normalize(pc.data.u.camPosition - inPosition); // TODO doesn't actually work, I messed something up

        vec3 halfDir = normalize(lightDir + viewDir);
        float specAngle = max(dot(halfDir, normal), 0.0);
        specular = pow(specAngle, shininess);
    }
    vec3 colorLinear = ambientColor + diffuseColor * lambertian * lightColor * lightPower / distance + specColor * specular * lightColor * lightPower / distance;
    vec3 colorGammaCorrected = pow(colorLinear, vec3(1.0 / screenGamma));
    outColor = texture(textures[textureIndex], inUV) * vec4(colorGammaCorrected, 1.0);
}