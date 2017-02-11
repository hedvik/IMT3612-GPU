#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VulkanAPIHandler.h"
#include "consts.h"

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

int main() {
	auto window = initWindow(WINDOW_WIDTH, WINDOW_HEIGHT);
	VulkanAPIHandler vulkanAPIHandler(window);
	setupResizeCallback(window, vulkanAPIHandler);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		vulkanAPIHandler.updateUniformBuffer();
		vulkanAPIHandler.drawFrame();
	}

	glfwDestroyWindow(window);

	return 0;
}