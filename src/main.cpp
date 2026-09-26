#include <iostream>

#include "engine.h"
#include "input.h"
#include "data.h"

struct {
    engine::TextureRef monke;
    engine::TextureRef kyaru;
    engine::TextureRef triangle;
} textures;

struct {
    engine::MeshRef monke;
    engine::MeshRef testQuad;
} meshes;

int main() {
    {
        textures.monke = engine::loadTexture("asset/monke.png");
        textures.kyaru = engine::loadTexture("asset/kyaru.png");
        textures.triangle = engine::loadTexture("asset/triangle.png");
    }
    {
        meshes.monke = engine::loadMeshObj("asset/monke.obj");
        meshes.testQuad = engine::loadTestMesh();
    }
	engine::init();
    input::init();
    data::init();
    input::Cam c = { .pos = glm::vec3(-5.0f, 0.0f, 1.0f), .pitch = 0, .yaw = 0, .fov = glm::radians(45.0f) };
    while (!glfwWindowShouldClose(engine::window)) {
        glfwPollEvents();
        data::updateTick();
        data::frameTick();
        input::pollMouseMovement();
        input::captureReleaseMouse();
        input::cameraMovement(c);
        engine::ShaderUniformData uniformData = {
                .proj = c.projMat(),
                .view = c.viewMat(),
                .camPosition = c.pos
        };
        engine::ShaderInstanceData instanceData[3] = {
            { .model = glm::rotate(glm::mat4(1.0f), data::time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)), .texture = textures.monke },
            { .model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 10.0f, sin(data::time) * 5.0f)), .texture = textures.triangle },
            { .model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -10.0f, sin(data::time + 3.0f) * 3.0f)), .texture = textures.kyaru }
        };
        engine::startDraw(uniformData, { .data = instanceData, .size = 3 });
        engine::drawMesh(meshes.monke, 2);
        engine::drawMesh(meshes.testQuad, 1);
        engine::endDraw();
    }
	engine::cleanup();
}