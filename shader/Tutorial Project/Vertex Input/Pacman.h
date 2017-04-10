#pragma once
#include "Moveable.h"
class Pacman : public Moveable {
public:
	Pacman(std::shared_ptr<RenderableMaze> ptr,
		   VulkanAPIHandler* vkAPIHandler,
		   glm::vec4 pos,
		   glm::vec3 renderableScale,
		   glm::vec4 c);
	void handleInput(GLFWKeyEvent event);
};

