#include <iostream>

#include "engine.h"

int main() {
	engine::init();
    while (!glfwWindowShouldClose(engine::window)) {
        glfwPollEvents();
        engine::drawFrame();
    }
	engine::cleanup();
}