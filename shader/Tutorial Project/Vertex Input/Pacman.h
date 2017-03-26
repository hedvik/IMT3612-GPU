#pragma once
#include "Moveable.h"
class Pacman : public Moveable {
public:
	Pacman(
		std::shared_ptr<RenderableMaze> ptr,
		VulkanAPIHandler* vkAPIHandler,
		glm::vec4 pos,
		std::string texPath,
		std::string meshPath,
		glm::vec3 renderableScale,
		glm::vec4 c = glm::vec4(1.f, 1.f, 1.f, 1.f),
		bool invertedNormals = false);
	void handleInput(GLFWKeyEvent event);
};

