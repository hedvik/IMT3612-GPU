#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm\glm.hpp>
#include <array>
#include <glm/gtx/hash.hpp>
#include "consts.h"

struct RenderableUBO {
	glm::mat4 mvp;
	glm::mat4 viewMatrix;
	glm::mat4 modelMatrix;
};

struct SceneUBO {
	glm::vec4 lightPositions[NUM_LIGHTS];
};

struct Vertex {
	glm::vec4 position;
	glm::vec4 color;
	glm::vec4 texCoord;
	glm::vec4 normal;

	static VkVertexInputBindingDescription getBindingDescription() {
		// The binding description describes at which rate the program will load data from memory throughout the vertices
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, NUM_VERTEX_ATTRIBUTES> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, NUM_VERTEX_ATTRIBUTES> attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, normal);

		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const {
		return position == other.position && color == other.color && texCoord == other.texCoord && normal == other.normal;
	}
};

// http://en.cppreference.com/w/cpp/utility/hash
namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.position) ^
				    (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				    (hash<glm::vec2>()(vertex.texCoord) << 1) ^
					(hash<glm::vec3>()(vertex.normal) >> 1);
		}
	};
}



