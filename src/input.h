#pragma once

#include "engine.h"

namespace input {

// angles in radians
struct Cam {
    glm::vec3 pos;
    f32 pitch;
    f32 yaw;
    f32 fov;

    glm::mat4 projMat() {
        glm::mat4 proj = glm::perspective(fov, engine::swapchainExtent.width / (f32)engine::swapchainExtent.height, 0.1f, 1000.0f);
        proj[1][1] *= -1;
        return proj;
    }
    glm::mat4 viewMat() {
        return glm::lookAt(pos, pos + dirLook(), dirUp());
    }
    glm::vec3 dirLook() {
        return glm::normalize(glm::vec3(cos(yaw) * cos(pitch), sin(yaw) * cos(pitch), sin(pitch)));
    }
    glm::vec3 dirForward() {
        return glm::normalize(glm::vec3(cos(yaw), sin(yaw), 0.0f));
    }
    glm::vec3 dirRight() {
        return -glm::normalize(glm::cross(dirForward(), dirUp()));
    }
    glm::vec3 dirUp() {
        return glm::vec3(0.0f, 0.0f, 1.0f);
    }
};

glm::dvec2 mousePos = {};
glm::dvec2 mouseDelta = {};
bool mouseCaptured = false;

glm::dvec2 scrollDelta = {};
bool scrollJustSet = false;

void scrollCallback(GLFWwindow* window, f64 xoffset, f64 yoffset) {
    scrollDelta.x = xoffset;
    scrollDelta.y = yoffset;
    scrollJustSet = true;
}

void init() {
    glfwSetScrollCallback(engine::window, scrollCallback);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(engine::window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
}

bool keyDown(i32 key) {
    return glfwGetKey(engine::window, key) == GLFW_PRESS || glfwGetKey(engine::window, key) == GLFW_REPEAT;
}

void pollMouseMovement() {
    glm::dvec2 mousePosNew = {};
    glfwGetCursorPos(engine::window, &mousePosNew.x, &mousePosNew.y);
    mouseDelta = mousePosNew - mousePos;
    mousePos = mousePosNew;

    if (scrollJustSet) { scrollJustSet = false; }
    else { scrollDelta = {}; }
}

void cameraMovement(Cam& c) {
    if (keyDown(GLFW_KEY_W)) {
        c.pos += c.dirForward() * 0.2f;
    }
    if (keyDown(GLFW_KEY_S)) {
        c.pos -= c.dirForward() * 0.2f;
    }
    if (keyDown(GLFW_KEY_A)) {
        c.pos += c.dirRight() * 0.2f;
    }
    if (keyDown(GLFW_KEY_D)) {
        c.pos -= c.dirRight() * 0.2f;
    }
    if (keyDown(GLFW_KEY_SPACE)) {
        c.pos += c.dirUp() * 0.2f;
    }
    if (keyDown(GLFW_KEY_LEFT_ALT)) {
        c.pos -= c.dirUp() * 0.2f;
    }
    if (mouseCaptured) {
        c.yaw -= f32(mouseDelta.x / 500.0);
        c.pitch -= f32(mouseDelta.y / 500.0);
        c.pitch = glm::clamp(c.pitch, glm::radians(-89.0f), glm::radians(89.0f));
    }
    if (scrollDelta.y != 0.0) {
        c.fov -= (f32)glm::radians(scrollDelta.y * 2.0);
        c.fov = glm::clamp(c.fov, glm::radians(1.0f), glm::radians(179.0f));
    }
}

void captureReleaseMouse() {
    if (keyDown(GLFW_KEY_ESCAPE)) {
        glfwSetInputMode(engine::window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        mouseCaptured = false;
    }
    if (glfwGetMouseButton(engine::window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        glfwSetInputMode(engine::window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        mouseCaptured = true;
    }
}

}