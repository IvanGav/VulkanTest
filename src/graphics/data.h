#pragma once

#include <deque>
#include "engine.h"

namespace data {

const u32 FPS_BUFFER_SIZE = 60;
const u32 TARGET_FPS = 60;
const u32 TARGET_UPS = 60;

f64 time = 0.0; // seconds since launch
f64 timeDelta = 0.0; // seconds last frame took
f64 updateTimeDelta = 0.0; // seconds last update took
u32 frame = 2; // frames since launch; starts at 2 because `input` thinks you pressed and released all buttons at frame 0 otherwise; fix later somehow
f64 fps = 0.0; // fps, averaged over past FPS_BUFFER_SIZE frames
u32 update = 0; // updates since launch
f64 ups = 0.0;
i32 windowWidth = 0; // logical window width
i32 windowHeight = 0; // logical window height
i32 framebufferWidth = 0; // actual framebuffer width
i32 framebufferHeight = 0; // actual framebuffer height

std::chrono::steady_clock::time_point startTime = {};

std::chrono::steady_clock::time_point frameStartTime = {};
std::deque<f64> frameTimes = {};
f64 frameTimesCumulative = 0.0;

std::chrono::steady_clock::time_point updateStartTime = {};
std::deque<f64> updateTimes = {};
f64 updateTimesCumulative = 0.0;

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
	timeDelta = std::chrono::duration<f64, std::chrono::seconds::period>(frameStartTime - lastFrameStartTime).count();
	frame += 1;
	if (frameTimes.size() >= FPS_BUFFER_SIZE) {
		frameTimesCumulative -= frameTimes.front();
		frameTimes.pop_front();
	}
	frameTimes.push_back(timeDelta);
	frameTimesCumulative += frameTimes.back();
	fps = frameTimes.size() / frameTimesCumulative;
	time = std::chrono::duration<f64, std::chrono::seconds::period>(frameStartTime - startTime).count();
}

void updateTick() {
	std::chrono::steady_clock::time_point lastUpdateStartTime = updateStartTime;
	updateStartTime = std::chrono::high_resolution_clock::now();
	updateTimeDelta = std::chrono::duration<f64, std::chrono::seconds::period>(updateStartTime - lastUpdateStartTime).count();
	update += 1;
	if (updateTimes.size() >= FPS_BUFFER_SIZE) {
		updateTimesCumulative -= updateTimes.front();
		updateTimes.pop_front();
	}
	updateTimes.push_back(updateTimeDelta);
	updateTimesCumulative += updateTimes.back();
	ups = updateTimes.size() / updateTimesCumulative;
}

f64 timeSinceLastUpdate() {
	std::chrono::steady_clock::time_point now = std::chrono::high_resolution_clock::now();
	f64 timePassed = std::chrono::duration<f64, std::chrono::seconds::period>(now - updateStartTime).count();
	return timePassed;
}

}