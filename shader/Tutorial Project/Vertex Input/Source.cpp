#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VulkanAPIHandler.h"
#include "consts.h"

// http://stackoverflow.com/questions/34141522/c-incorrect-fps-and-deltatime-measuring-using-stdchrono
using ms = std::chrono::duration<float, std::milli>;

GLFWwindow* initWindow(int width, int height) {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	auto window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
	return window;
}

void setupResizeCallback(GLFWwindow* window, VulkanAPIHandler &app) {
	glfwSetWindowUserPointer(window, app.getPtr());
	glfwSetWindowSizeCallback(window, VulkanAPIHandler::onWindowResized);
}

static void inputCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	VulkanAPIHandler* apiHandler = reinterpret_cast<VulkanAPIHandler*>(glfwGetWindowUserPointer(window));
	apiHandler->handleInput(GLFWKeyEvent(window, key, scancode, action, mods));
}

int main() {
	auto window = initWindow(WINDOW_WIDTH, WINDOW_HEIGHT);
	VulkanAPIHandler vulkanAPIHandler(window);
	setupResizeCallback(window, vulkanAPIHandler);
	glfwSetKeyCallback(window, inputCallback);
	glfwSetWindowUserPointer(window, &vulkanAPIHandler);

	auto lastTime = std::chrono::high_resolution_clock::now();
	while (!glfwWindowShouldClose(window)) {
		auto currentTime = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration_cast<ms>(currentTime - lastTime).count();
		lastTime = currentTime;

		glfwPollEvents();
		vulkanAPIHandler.updateUniformBuffers();
		vulkanAPIHandler.update(deltaTime);
		vulkanAPIHandler.drawFrame();
	}

	glfwDestroyWindow(window);

	return 0;
}