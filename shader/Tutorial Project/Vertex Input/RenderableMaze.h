#pragma once
#include "Renderable.h"
#include "Structs.h"
#include "tinyxml2.h"

class RenderableMaze : public Renderable {
public:
	RenderableMaze(VulkanAPIHandler* vkAPIHandler, glm::vec4 pos, std::string texPath);
	~RenderableMaze();
private:
	const int WALL_HEIGHT{30};
	const float FLOOR_OFFSET_Y{-1.5f};
	const std::string FILE_PATH{"SVGs/level1.svg"};

	const int NUM_VERTICES_PER_FACE{4};
	const int NUM_INDICES_PER_FACE{6};
	const int NUM_FACES_PER_PRISM{6};
	const int NUM_VERTICES_PER_PRISM{NUM_VERTICES_PER_FACE * NUM_FACES_PER_PRISM};
	const int NUM_INDICES_PER_PRISM{NUM_INDICES_PER_FACE * NUM_FACES_PER_PRISM};
	
	int vertexWallAmount{0};
	int numIndices{0};
	int numVertices{0};

	std::vector<CollisionRect> wallCollisionRects{};
	std::vector<glm::vec4> textureCoordinates{
		{ 0.0f, 0.0f, 0.0f, 1.0f },
		{ 0.0f, 1.0f, 0.0f, 1.0f },
		{ 1.0f, 1.0f, 0.0f, 1.0f },
		{ 1.0f, 0.0f, 0.0f, 1.0f }
	};

	void readSVGRects(const char* fileName);
	void convertRectsToVertices();
	void pushVertexFace(std::vector<glm::vec4> vertices, glm::vec4 color = glm::vec4(0, 0, 1, 1));
	void addFloorVertices();
	void pushIndices();
};

