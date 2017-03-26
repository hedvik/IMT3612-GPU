#include "Moveable.h"



Moveable::Moveable(
	std::shared_ptr<RenderableMaze> ptr, 
	VulkanAPIHandler* vkAPIHandler, 
	glm::vec4 pos, 
	std::string texPath, 
	std::string meshPath, 
	glm::vec3 renderableScale, 
	glm::vec4 c, 
	bool invertedNormals) : Renderable(vkAPIHandler, pos, texPath, meshPath, renderableScale, c, invertedNormals) {
	mazePtr = ptr;
	setupCollider();
}

Moveable::~Moveable() {
}

void Moveable::update(float deltaTime) {
	bool collision = false;
	glm::vec2 newPosition;

	if (velocity.x != 0 || velocity.y != 0) {
		newPosition.x = velocity.x * deltaTime + position.x;
		newPosition.y = velocity.y * deltaTime + position.z;
		collisionRect.x = newPosition.x + lowestX;
		collisionRect.y = newPosition.y + lowestZ;

		for (auto& wall : mazePtr->getWalls()) {
			if (CollisionHandler::checkCollision(collisionRect, wall)) {
				collision = true;
			}
		}

		if (!collision) {
			position.x = newPosition.x;
			position.z = newPosition.y;
		} 
	}
}

void Moveable::moveTowardsDirection(MoveableDirections direction) {
	if (direction == MoveableDirections::MOVEABLE_DIR_UP) {
		velocity.y = -movementSpeed;
		velocity.x = 0;
	}
	else if (direction == MoveableDirections::MOVEABLE_DIR_DOWN) {
		velocity.y = movementSpeed;
		velocity.x = 0;
	}
	else if (direction == MoveableDirections::MOVEABLE_DIR_RIGHT) {
		velocity.x = movementSpeed;
		velocity.y = 0;
	}
	else if (direction == MoveableDirections::MOVEABLE_DIR_LEFT) {
		velocity.x = -movementSpeed;
		velocity.y = 0;
	}
}

void Moveable::setupCollider() {
	// Finding collider bounds
	auto minMaxX = std::minmax_element(vertices.begin(), vertices.end(), [](const Vertex& a, const Vertex& b) { return a.position.x < b.position.x; });
	auto minMaxZ = std::minmax_element(vertices.begin(), vertices.end(), [](const Vertex& a, const Vertex& b) { return a.position.z < b.position.z; });
	lowestX = (*minMaxX.first).position.x * scale.x;
	highestX = (*minMaxX.second).position.x * scale.x;
	lowestZ = (*minMaxZ.first).position.z * scale.z;
	highestZ = (*minMaxZ.second).position.z * scale.z;

	collisionRect.x = position.x + lowestX;
	collisionRect.y = position.z + lowestZ;
	if (lowestX < 0 || lowestZ < 0) {
		collisionRect.w = abs(lowestX) + highestX;
		collisionRect.h = abs(lowestZ) + highestZ;
	}
	else {
		collisionRect.w = highestX;
		collisionRect.h = highestZ;
	}
}
