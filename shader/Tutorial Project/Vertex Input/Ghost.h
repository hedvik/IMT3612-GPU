#pragma once
#include "Moveable.h"

class Ghost : public Moveable {
public:
	Ghost(
		SceneUBO* uboPtr, 
		std::shared_ptr<RenderableMaze> mazePtr, 
		VulkanAPIHandler* vkAPIHandler, 
		int lightIndex, 
		glm::vec4 pos, 
		glm::vec3 renderableScale, 
		glm::vec4 c);
	~Ghost();
protected:
	virtual void update(float dt);
private:
	const float INTERPOLATION_SPEED{0.001f};
	float interpolationTimer{0};

	float baseY{position.y};
	float offsetY{baseY + 40};

	int sceneUBOLightIndex{0};
	SceneUBO* sceneUBOptr;
};

