#include "Ghost.h"

Ghost::Ghost(
		SceneUBO* uboPtr,
		std::shared_ptr<RenderableMaze> mazePtr, 
		VulkanAPIHandler* vkAPIHandler, 
		int lightIndex, 
		glm::vec4 pos, 
		glm::vec3 renderableScale, 
		glm::vec4 c) : Moveable(mazePtr, vkAPIHandler, pos, DEFAULT_TEXTURE_PATH, CUBE_MODEL_PATH, renderableScale, c, true) {
	sceneUBOLightIndex = lightIndex;
	sceneUBOptr = uboPtr;

	sceneUBOptr->lightColors[sceneUBOLightIndex] = c;
	material.diffuseGain = 5;
}

Ghost::~Ghost() {
}

void Ghost::update(float dt) {
	Moveable::update(dt);

	interpolationTimer += INTERPOLATION_SPEED * dt;

	position.y = baseY * pow(cos(interpolationTimer), 2) + (offsetY) * pow(sin(interpolationTimer), 2);

	sceneUBOptr->lightPositions[sceneUBOLightIndex] = glm::vec4(position, 1);
}
