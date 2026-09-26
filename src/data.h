#pragma once

#include "engine.h"

namespace data {

f32 time = 0.0; // seconds since launch
u32 updates = 0; // updates since launch
u32 frames = 0; // frames since launch
f32 ups = 0.0f;
f32 fps = 0.0f;
i32 windowWidth = 0; // logical window width
i32 windowHeight = 0; // logical window height
i32 framebufferWidth = 0; // actual framebuffer width
i32 framebufferHeight = 0; // actual framebuffer height

std::chrono::steady_clock::time_point startTime = {};

void init() {
	startTime = std::chrono::high_resolution_clock::now();
}

void framebufferResizedCallback() {
	glfwGetWindowSize(engine::window, &windowWidth, &windowHeight);
	glfwGetFramebufferSize(engine::window, &framebufferWidth, &framebufferHeight);
}

void updateTick() {
	std::chrono::steady_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
	time = std::chrono::duration<f32, std::chrono::seconds::period>(currentTime - startTime).count();
	updates += 1;
}

void frameTick() {
	frames += 1;
}

}