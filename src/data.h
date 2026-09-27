#pragma once

#include <deque>
#include "engine.h"

namespace data {

const u32 FPS_BUFFER_SIZE = 60;

f32 time = 0.0; // seconds since launch
f32 timeDelta = 0.0; // seconds last frame took
u32 frames = 0; // frames since launch
f32 fps = 0.0f; // fps, averaged over past FPS_BUFFER_SIZE frames
i32 windowWidth = 0; // logical window width
i32 windowHeight = 0; // logical window height
i32 framebufferWidth = 0; // actual framebuffer width
i32 framebufferHeight = 0; // actual framebuffer height

std::chrono::steady_clock::time_point startTime = {};
std::chrono::steady_clock::time_point frameStartTime = {};
std::deque<f32> frameTimes = {};
f32 frameTimesCumulative = 0.0f;

void init() {
	startTime = std::chrono::high_resolution_clock::now();
}

void framebufferResizedCallback() {
	glfwGetWindowSize(engine::window, &windowWidth, &windowHeight);
	glfwGetFramebufferSize(engine::window, &framebufferWidth, &framebufferHeight);
}

void frameTick() {
	std::chrono::steady_clock::time_point lastFrameStartTime = frameStartTime;
	frameStartTime = std::chrono::high_resolution_clock::now();
	time = std::chrono::duration<f32, std::chrono::seconds::period>(frameStartTime - startTime).count();
	timeDelta = std::chrono::duration<f32, std::chrono::seconds::period>(frameStartTime - lastFrameStartTime).count();
	frames += 1;
	if (frameTimes.size() >= FPS_BUFFER_SIZE) {
		frameTimesCumulative -= frameTimes.front();
		frameTimes.pop_front();
	}
	frameTimes.push_back(timeDelta);
	frameTimesCumulative += frameTimes.back();
	fps = frameTimes.size() / frameTimesCumulative;
}

}