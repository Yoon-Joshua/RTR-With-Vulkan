#include "application.h"
#include "timer.h"
#include <GLFW/glfw3.h>

int main() {
	Timer timer;
	Application app;

	app.init();
	app.mainLoop(timer);
	app.cleanup();
}