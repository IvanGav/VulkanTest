#include <iostream>

#include "engine.h"

int main() {
	engine::Engine e = {};
	e.init();
	e.run();
	e.cleanup();
}