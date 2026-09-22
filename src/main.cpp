#include <iostream>

#include "engine.h"

int main() {
	engine::Engine e = {};
	e.init(2);
	e.run();
	e.cleanup();
}