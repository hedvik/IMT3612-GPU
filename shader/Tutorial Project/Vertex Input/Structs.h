#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm\glm.hpp>
#include <array>
#include <glm/gtx/hash.hpp>
#include "consts.h"

struct RenderableUBO {
	glm::mat4 mvp;
	glm::mat4 projectionMatrix;
	glm::mat4 viewMatrix;
	glm::mat4 modelMatrix;
};

struct RenderableMaterialUBO {
	float specularExponent{128.0};
	float specularGain{1};
	float diffuseGain{1};
	bool selfShadowEnabled{true};
};

struct SceneUBO {
	glm::mat4 projectionMatrix;
	glm::mat4 lightOffsetMatrices[NUM_LIGHTS];
	glm::vec4 lightPositions[NUM_LIGHTS];
	glm::vec4 lightColors[NUM_LIGHTS];
};

struct PushConstants {
	glm::mat4 vireMatrix;
	int currentMatrixIndex;

	PushConstants(glm::mat4 view, int index) {
		vireMatrix = view;
		currentMatrixIndex = index;
	}
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

// OffscreenPass and FrameBufferAttachment are used for shadow mapping
struct FrameBufferAttachment {
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
};

enum RenderableType {
	RENDERABLE_PACMAN = 0,
	RENDERABLE_MAZE,
	RENDERABLE_GHOST
};

struct RenderableInformation {
	bool castShadows{ true };
	RenderableType type;

	RenderableInformation(RenderableType renderType) {
		type = renderType;
	}

	RenderableInformation(RenderableType renderType, bool castShadow) {
		castShadows = castShadow;
		type = renderType;
	}
};

struct OffscreenPass {
	int32_t width, height;
	VkFramebuffer frameBuffer;
	FrameBufferAttachment color, depth;
	VkRenderPass renderPass;
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	// Semaphore used to synchronize between offscreen and final scene render pass
	VkSemaphore semaphore = VK_NULL_HANDLE;
};

struct CollisionRect{
	int x{0};
	int y{0};
	int h{0};
	int w{0};

	CollisionRect() {};

	CollisionRect(int x, int y, int h, int w) {
		this->x = x;
		this->y = y;
		this->h = h;
		this->w = w;
	}
};

struct GLFWKeyEvent {
	GLFWwindow* window;
	int key;
	int scancode;
	int action;
	int mods;

	GLFWKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods) {
		this->window = window;
		this->key = key;
		this->scancode = scancode;
		this->action = action;
		this->mods = mods;
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