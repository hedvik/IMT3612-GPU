#pragma once
#include "Moveable.h"
#include "Pacman.h"

#include <random>

enum Quadrants {
	QUADRANT_TOP_LEFT = 0,
	QUADRANT_TOP_RIGHT,
	QUADRANT_BOTTOM_LEFT,
	QUADRANT_BOTTOM_RIGHT
};

class Ghost : public Moveable {
public:
	Ghost(
		SceneUBO* uboPtr, 
		std::shared_ptr<Pacman> pacPtr,
		std::shared_ptr<RenderableMaze> mazePtr, 
		VulkanAPIHandler* vkAPIHandler, 
		int lightIndex, 
		glm::vec4 pos, 
		glm::vec3 renderableScale, 
		glm::vec4 c);
	~Ghost();

	void respawn(glm::vec3 respawnPosition);
protected:
	virtual void update(float dt);
private:
	const float INTERPOLATION_SPEED{0.001f};
	float interpolationTimer{0};

	const float DISTANCE_FROM_PACMAN_THRESHOLD = 300.f;
	const float MOVEMENT_DECISION_FREQUENCY{1.f};
	float movementDecisionTimer{0};

	float baseY{position.y};
	float offsetY{baseY + 40};

	int sceneUBOLightIndex{0};
	SceneUBO* sceneUBOPtr;
	std::shared_ptr<Pacman> pacmanPtr;

	void ghostMovement();
	bool checkForPacmanPlacement(Quadrants quadrant);

	void chooseDirection(MoveableDirections choice1, MoveableDirections choice2);
	void chooseDirection();
};

