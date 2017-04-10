#pragma once
#include <random>
#include "Moveable.h"
#include "Pacman.h"
#include "Utilities.h"

enum Quadrants {
	QUADRANT_TOP_LEFT = 0,
	QUADRANT_TOP_RIGHT,
	QUADRANT_BOTTOM_LEFT,
	QUADRANT_BOTTOM_RIGHT
};

class Ghost : public Moveable {
public:
	Ghost(SceneUBO* uboPtr, 
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

	const float DISTANCE_FROM_PACMAN_THRESHOLD = 300.f;
	const float MOVEMENT_DECISION_FREQUENCY{1.f};
	
	// The base and and target y components of the ghost's position that we interpolate with
	const float BASE_Y_POSITION{position.y};
	const float OFFSET_Y_POSITION{BASE_Y_POSITION + 40};

	float interpolationTimer{0};
	float movementDecisionTimer{0};

	int sceneUBOLightIndex{0};
	SceneUBO* sceneUBOPtr;
	std::shared_ptr<Pacman> pacmanPtr;

	void ghostMovement();
	bool checkForPacmanPlacement(Quadrants quadrant);

	void chooseDirection(MoveableDirections choice1, MoveableDirections choice2);
	void chooseDirection();
};

