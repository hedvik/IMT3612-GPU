#pragma once
#include <algorithm>
#include "Renderable.h"
#include "CollisionHandler.h"
#include "RenderableMaze.h"

enum MoveableDirections {
	MOVEABLE_DIR_UP = 0,
	MOVEABLE_DIR_DOWN,
	MOVEABLE_DIR_LEFT,
	MOVEABLE_DIR_RIGHT
};

class Moveable : public Renderable {
public:
	Moveable(std::shared_ptr<RenderableMaze> ptr,
			 VulkanAPIHandler* vkAPIHandler,
			 glm::vec4 pos,
			 std::string texPath,
			 std::string meshPath,
			 glm::vec3 renderableScale,
			 glm::vec4 c = glm::vec4(1.f, 1.f, 1.f, 1.f),
			 bool invertedNormals = false);
	~Moveable();

	virtual void update(float deltaTime);

	CollisionRect getCollisionRect();
protected:
	float movementSpeed{0.25f};
	void moveTowardsDirection(MoveableDirections direction);
	glm::vec2 velocity{0, 0};
private:
	float lowestX{0};
	float lowestZ{0};
	float highestX{0};
	float highestZ{0};

	std::shared_ptr<RenderableMaze> mazePtr;
	CollisionRect collisionRect;

	void setupCollider();
};

