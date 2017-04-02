#include "RenderableMaze.h"



RenderableMaze::RenderableMaze(VulkanAPIHandler* vkAPIHandler, glm::vec4 pos) : Renderable(vkAPIHandler, pos) {
	readSVGRects(FILE_PATH.c_str());
	convertRectsToVertices();
	addFloorVertices();

	// We add the 4 extra vertices and 6 extra indices from the floor to this calculation
	numVertices = NUM_VERTICES_PER_PRISM * wallCollisionRects.size() + NUM_VERTICES_PER_FACE;
	numIndices = NUM_INDICES_PER_PRISM * wallCollisionRects.size() + NUM_INDICES_PER_FACE;

	pushIndices();
}


RenderableMaze::~RenderableMaze() {
}

std::vector<CollisionRect> RenderableMaze::getWalls() {
	return wallCollisionRects;
}

void RenderableMaze::readSVGRects(const char* fileName) {
	tinyxml2::XMLDocument doc;
	doc.LoadFile(fileName);

	CollisionRect wall;
	int x = 0, y = 0;
	int width = 0, height = 0;

	tinyxml2::XMLElement *levelElement = doc.FirstChildElement("svg")->FirstChildElement("g");

	for (tinyxml2::XMLElement* child = levelElement->FirstChildElement(); child != NULL; child = child->NextSiblingElement()) {
		const char* name = child->Attribute("id");
		x = child->IntAttribute("x");
		y = child->IntAttribute("y");
		width = child->IntAttribute("width");
		height = child->IntAttribute("height");
		printf("Rect: %s %d %d %d %d\n", name, x, y, width, height);

		wall.x = x;
		wall.y = y;
		wall.h = height;
		wall.w = width;
		wallCollisionRects.push_back(wall);
	}
}

void RenderableMaze::convertRectsToVertices() {
	std::vector<glm::vec4> vertices(4);

	// Converts the 2D wall data to 3D and adds these as vertices
	for (auto& wall : wallCollisionRects) {
		// Front (lets say you are looking at the left side from the camera)
		vertices[0] = { wall.x, 0,			  wall.y,		   1.f };
		vertices[1] = { wall.x, 0,			  wall.y + wall.h, 1.f };
		vertices[2] = { wall.x, WALL_HEIGHT,  wall.y + wall.h, 1.f };
		vertices[3] = { wall.x, WALL_HEIGHT,  wall.y,          1.f };
		pushVertexFace(vertices);

		// Right
		vertices[0] = { wall.x,          0,		      wall.y + wall.h, 1.f };
		vertices[1] = { wall.x + wall.w, 0,		      wall.y + wall.h, 1.f };
		vertices[2] = { wall.x + wall.w, WALL_HEIGHT, wall.y + wall.h, 1.f };
		vertices[3] = { wall.x,			 WALL_HEIGHT, wall.y + wall.h, 1.f };
		pushVertexFace(vertices);

		// Back
		vertices[0] = { wall.x + wall.w, 0,		      wall.y + wall.h, 1.f };
		vertices[1] = { wall.x + wall.w, 0,		      wall.y,		   1.f };
		vertices[2] = { wall.x + wall.w, WALL_HEIGHT, wall.y,		   1.f };
		vertices[3] = { wall.x + wall.w, WALL_HEIGHT, wall.y + wall.h, 1.f };
		pushVertexFace(vertices);

		// Left
		vertices[0] = { wall.x + wall.w, 0,		      wall.y, 1.f };
		vertices[1] = { wall.x,			 0,		      wall.y, 1.f };
		vertices[2] = { wall.x,			 WALL_HEIGHT, wall.y, 1.f };
		vertices[3] = { wall.x + wall.w, WALL_HEIGHT, wall.y, 1.f };
		pushVertexFace(vertices);

		// Top
		vertices[0] = { wall.x,          WALL_HEIGHT, wall.y,          1.f };
		vertices[1] = { wall.x,			 WALL_HEIGHT, wall.y + wall.h, 1.f };
		vertices[2] = { wall.x + wall.w, WALL_HEIGHT, wall.y + wall.h, 1.f };
		vertices[3] = { wall.x + wall.w, WALL_HEIGHT, wall.y,          1.f };
		pushVertexFace(vertices);

		// Bottom
		vertices[0] = { wall.x + wall.w, 0, wall.y,          1.f };
		vertices[1] = { wall.x + wall.w, 0, wall.y + wall.h, 1.f };
		vertices[2] = { wall.x,          0, wall.y + wall.h, 1.f };
		vertices[3] = { wall.x,          0, wall.y,          1.f };
		pushVertexFace(vertices);
	}
}

void RenderableMaze::pushVertexFace(std::vector<glm::vec4> vertexPositions, glm::vec4 color) {
	std::vector<glm::vec4> normals(vertexPositions.size());

	// Calculating face normals
	glm::vec3 v0 = vertexPositions[0];
	glm::vec3 v1 = vertexPositions[1];
	glm::vec3 v2 = vertexPositions[2];
	glm::vec3 v3 = vertexPositions[3];

	glm::vec3 normal012 = glm::normalize(glm::cross(v2 - v0, v1 - v2));
	glm::vec3 normal023 = glm::normalize(glm::cross(v3 - v0, v2 - v0));

	// The average of the two triangle normals gives us the face normal
	glm::vec4 faceNormal(-(normal012 + normal023) * 0.5f, 0.0f);

	for (std::vector<int>::size_type i = 0; i != vertexPositions.size(); i++) {
		Vertex vertex;
		vertex.position = vertexPositions[i];
		vertex.color = color;
		vertex.normal = faceNormal;
		vertex.texCoord = textureCoordinates[i];
		vertices.push_back(vertex);
	}
}

void RenderableMaze::addFloorVertices() {
	// Finds the lowest and highest x/y values so we know where to put our floor plane
	auto minX = std::min_element(wallCollisionRects.begin(), wallCollisionRects.end(), [](const CollisionRect& a, const CollisionRect& b) { return a.x < b.x; });
	auto maxX = std::max_element(wallCollisionRects.begin(), wallCollisionRects.end(), [](const CollisionRect& a, const CollisionRect& b) { return a.x + a.w < b.x + b.w; });
	auto minY = std::min_element(wallCollisionRects.begin(), wallCollisionRects.end(), [](const CollisionRect& a, const CollisionRect& b) { return a.y < b.y; });
	auto maxY = std::max_element(wallCollisionRects.begin(), wallCollisionRects.end(), [](const CollisionRect& a, const CollisionRect& b) { return a.y + a.h < b.y + b.h; });
	
	int highestX{ (*maxX).x + (*maxX).w };
	int highestY{ (*maxY).y + (*maxY).h };
	int lowestX{ (*minX).x };
	int lowestY{ (*minY).y };

	std::vector<glm::vec4> vertices{ 
		{lowestX, FLOOR_OFFSET_Y, highestY, 1.f},
		{highestX, FLOOR_OFFSET_Y, highestY, 1.f},
		{highestX, FLOOR_OFFSET_Y, lowestY, 1.f},
		{lowestX, FLOOR_OFFSET_Y, lowestX, 1.f}
	};

	pushVertexFace(vertices, glm::vec4(0.1f, 0.1f, 0.1f, 1.f));
}

void RenderableMaze::pushIndices() {
	// Adding in the index order for all vertices with the order being (k,k+1,k+2  k,k+2,k+3) 
	int k = 0;
	for (int i = 0; i < numIndices; i += 6) {
		indices.push_back(k);
		indices.push_back(k + 1);
		indices.push_back(k + 2);

		indices.push_back(k);
		indices.push_back(k + 2);
		indices.push_back(k + 3);
		k += 4;
	}
}
