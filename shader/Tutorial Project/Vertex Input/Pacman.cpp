#include "Pacman.h"
#include "VulkanAPIHandler.h"

Pacman::Pacman(
		std::shared_ptr<RenderableMaze> ptr, 
		VulkanAPIHandler* vkAPIHandler, 
		glm::vec4 pos, 
		std::string texPath, 
		std::string meshPath, 
		glm::vec3 renderableScale, 
		glm::vec4 c, 
		bool invertedNormals) : Moveable(ptr, vkAPIHandler, pos, texPath, meshPath, renderableScale, c, invertedNormals) {
}

// based on https://www.libsdl.org/release/SDL-1.2.15/docs/html/guideinputkeyboard.html
void Pacman::handleInput(GLFWKeyEvent event) {
	if (event.action == GLFW_PRESS) {
		switch (event.key) {
		case GLFW_KEY_W:
			moveTowardsDirection(MoveableDirections::MOVEABLE_DIR_UP);
			break;
		case GLFW_KEY_A:
			moveTowardsDirection(MoveableDirections::MOVEABLE_DIR_LEFT);
			break;
		case GLFW_KEY_S:
			moveTowardsDirection(MoveableDirections::MOVEABLE_DIR_DOWN);
			break;
		case GLFW_KEY_D:
			moveTowardsDirection(MoveableDirections::MOVEABLE_DIR_RIGHT);
			break;
		default:
			break;
		}
	}
	else if (event.action == GLFW_RELEASE) {
		switch (event.key) {
		case GLFW_KEY_W:
			if (velocity.y < 0) {
				velocity.y = 0;
			}
			break;
		case GLFW_KEY_A:
			if (velocity.x < 0) {
				velocity.x = 0;
			}
			break;
		case GLFW_KEY_S:
			if (velocity.y > 0) {
				velocity.y = 0;
			}
			break;
		case GLFW_KEY_D:
			if (velocity.x > 0) {
				velocity.x = 0;
			}
			break;
		default:
			break;
		}
	}
}
