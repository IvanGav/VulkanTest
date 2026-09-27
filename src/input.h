#pragma once

#include "engine.h"
#include "data.h"

namespace input {

// how much a key has to be held down for to count as being "held"
const f32 KEY_HELD_DELAY = 0.15;

glm::dvec2 mousePos = {};
glm::dvec2 mouseDelta = {};

glm::dvec2 scrollDelta = {};
bool scrollJustSet = false;

u32 keyDownFrame[GLFW_KEY_LAST] = {};
u32 keyUpFrame[GLFW_KEY_LAST] = {};
f64 keyTimestamp[GLFW_KEY_LAST] = {};
u32 mouseButtonDownFrame[GLFW_MOUSE_BUTTON_LAST] = {};
u32 mouseButtonUpFrame[GLFW_MOUSE_BUTTON_LAST] = {};

void keyCallback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods) {
    switch (action) {
    case GLFW_PRESS: {
        keyDownFrame[key] = data::frame;
        keyTimestamp[key] = data::time;
        break;
    }
    case GLFW_RELEASE: {
        keyUpFrame[key] = data::frame;
        keyTimestamp[key] = data::time;
        break;
    }
    }
}

void mouseButtonCallback(GLFWwindow* window, i32 button, i32 action, i32 mods) {
    switch (action) {
    case GLFW_PRESS: {
        mouseButtonDownFrame[button] = data::frame;
        break;
    }
    case GLFW_RELEASE: {
        mouseButtonUpFrame[button] = data::frame;
        break;
    }
    }
}

void scrollCallback(GLFWwindow* window, f64 xoffset, f64 yoffset) {
    scrollDelta.x = xoffset;
    scrollDelta.y = yoffset;
    scrollJustSet = true;
}

void init() {
    glfwSetKeyCallback(engine::window, keyCallback);
    glfwSetMouseButtonCallback(engine::window, mouseButtonCallback);
    glfwSetScrollCallback(engine::window, scrollCallback);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(engine::window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    glfwSetInputMode(engine::window, GLFW_STICKY_KEYS, GLFW_TRUE);
}

// Make sure to call this after data::frameTick in this frame, because `*Released` and `*Pressed` will not work properly otherwise
void frameTick() {
    glfwPollEvents();
    glm::dvec2 mousePosNew = {};
    glfwGetCursorPos(engine::window, &mousePosNew.x, &mousePosNew.y);
    mouseDelta = mousePosNew - mousePos;
    mousePos = mousePosNew;

    if (scrollJustSet) { scrollJustSet = false; }
    else { scrollDelta = {}; }
}

// true as long as key is down
bool keyDown(i32 key) {
    return keyDownFrame[key] > keyUpFrame[key] || (keyDownFrame[key] == keyUpFrame[key] && data::frame == keyDownFrame[key]);
}
// true as long as key is up
bool keyUp(i32 key) {
    return !keyDown(key);
}
// guaranteed to be true for exactly 1 frame after the key was actually pressed
bool keyPressed(i32 key) {
    return data::frame == keyDownFrame[key];
}
// guaranteed to be true for exactly 1 frame after the key was actually released
bool keyReleased(i32 key) {
    if (keyUpFrame[key] == keyDownFrame[key]) { return data::frame == (keyUpFrame[key] + 1); }
    return data::frame == keyUpFrame[key];
}
// true when key held for some amount of time
bool keyHeld(i32 key) {
    return keyDown(key) && (data::time - keyTimestamp[key] >= KEY_HELD_DELAY);
}
// true as long as mouse button is down
bool mouseButtonDown(i32 mouseButton) {
    return mouseButtonDownFrame[mouseButton] > mouseButtonUpFrame[mouseButton] || (mouseButtonDownFrame[mouseButton] == mouseButtonUpFrame[mouseButton] && data::frame == mouseButtonDownFrame[mouseButton]);
}
// true as long as mouse button is up
bool mouseButtonUp(i32 mouseButton) {
    return !mouseButtonDown(mouseButton);
}
// guaranteed to be true for exactly 1 frame after the mouse button was actually pressed
bool mouseButtonPressed(i32 mouseButton) {
    return data::frame == mouseButtonDownFrame[mouseButton];
}
// guaranteed to be true for exactly 1 frame after the mouse button was actually released
bool mouseButtonReleased(i32 mouseButton) {
    if (mouseButtonUpFrame[mouseButton] == mouseButtonDownFrame[mouseButton]) { return data::frame == (mouseButtonUpFrame[mouseButton] + 1); }
    return data::frame == mouseButtonUpFrame[mouseButton];
}
}