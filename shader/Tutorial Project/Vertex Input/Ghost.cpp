#include "Ghost.h"

Ghost::Ghost(
		SceneUBO* uboPtr,
		std::shared_ptr<Pacman> pacPtr,
		std::shared_ptr<RenderableMaze> mazePtr, 
		VulkanAPIHandler* vkAPIHandler, 
		int lightIndex, 
		glm::vec4 pos, 
		glm::vec3 renderableScale, 
		glm::vec4 c) : Moveable(mazePtr, vkAPIHandler, pos, DEFAULT_TEXTURE_PATH, CUBE_MODEL_PATH, renderableScale, c, true) {
	sceneUBOLightIndex = lightIndex;
	sceneUBOPtr = uboPtr;
	pacmanPtr = pacPtr;

	sceneUBOPtr->lightColors[sceneUBOLightIndex] = c;
	material.diffuseGain = 5;
}

Ghost::~Ghost() {
}

void Ghost::update(float dt) {
	Moveable::update(dt);

	interpolationTimer += INTERPOLATION_SPEED * dt;
	
	// delta time is measured in milliseconds
	movementDecisionTimer += dt / 1000.f;

	position.y = baseY * pow(cos(interpolationTimer), 2) + (offsetY) * pow(sin(interpolationTimer), 2);
	sceneUBOPtr->lightPositions[sceneUBOLightIndex] = glm::vec4(position, 1);

	if (movementDecisionTimer >= MOVEMENT_DECISION_FREQUENCY) {
		movementDecisionTimer = 0;
		ghostMovement();
	}
}

void Ghost::ghostMovement() {
	glm::vec3 pacmanPosition = pacmanPtr->getPosition();
	glm::vec3 vectorPacmanGhost = pacmanPosition - position;
	float distance = sqrt(pow(vectorPacmanGhost.x, 2) + pow(vectorPacmanGhost.y, 2) + pow(vectorPacmanGhost.z, 2));

	if (distance <= DISTANCE_FROM_PACMAN_THRESHOLD) {
		if (checkForPacmanPlacement(QUADRANT_TOP_LEFT)) {
			chooseDirection(MOVEABLE_DIR_DOWN, MOVEABLE_DIR_RIGHT);
		}
		else if (checkForPacmanPlacement(QUADRANT_TOP_RIGHT)) {
			chooseDirection(MOVEABLE_DIR_DOWN, MOVEABLE_DIR_LEFT);
		}
		else if (checkForPacmanPlacement(QUADRANT_BOTTOM_LEFT)) {
			chooseDirection(MOVEABLE_DIR_UP, MOVEABLE_DIR_RIGHT);
		}
		else if (checkForPacmanPlacement(QUADRANT_BOTTOM_RIGHT)) {
			chooseDirection(MOVEABLE_DIR_UP, MOVEABLE_DIR_LEFT);
		}

		// if pacman is right above/below/to the right or left, flees in the opposite direction
		else if (pacmanPosition.x < position.x && pacmanPosition.z == position.z) {
			moveTowardsDirection(MOVEABLE_DIR_RIGHT);
		}

		else if (pacmanPosition.x > position.x && pacmanPosition.z == position.z) {
			moveTowardsDirection(MOVEABLE_DIR_LEFT);
		}

		else if (pacmanPosition.x == position.x && pacmanPosition.z < position.z) {
			moveTowardsDirection(MOVEABLE_DIR_DOWN);
		}

		else if (pacmanPosition.x == position.x && pacmanPosition.z > position.z) {
			moveTowardsDirection(MOVEABLE_DIR_UP);
		}
	}
	else {
		chooseDirection();
	}
}

bool Ghost::checkForPacmanPlacement(Quadrants quadrant) {
	glm::vec3 pacmanPosition = pacmanPtr->getPosition();
	
	switch (quadrant) {
	case QUADRANT_TOP_LEFT:
		return (pacmanPosition.x < position.x && pacmanPosition.z < position.z) ? true : false;
	case QUADRANT_TOP_RIGHT:
		return (pacmanPosition.x > position.x && pacmanPosition.z < position.z) ? true : false;
	case QUADRANT_BOTTOM_LEFT:
		return (pacmanPosition.x < position.x && pacmanPosition.z > position.z) ? true : false;
	case QUADRANT_BOTTOM_RIGHT:
		return (pacmanPosition.x > position.x && pacmanPosition.z > position.z) ? true : false;
	}
}

void Ghost::chooseDirection(MoveableDirections choice1, MoveableDirections choice2) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, 1);

	MoveableDirections chosenDir = dis(gen) ? choice1 : choice2;
	moveTowardsDirection(chosenDir);
}

void Ghost::chooseDirection() {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, 3);

	MoveableDirections chosenDir = static_cast<MoveableDirections>(dis(gen));
	moveTowardsDirection(chosenDir);
}

void Ghost::respawn(glm::vec3 respawnPosition) {
	position = respawnPosition;
}
