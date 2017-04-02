#pragma once
#include <memory>
#include <vector>
#include <glm\glm.hpp>
#include "Renderable.h"
#include "RenderableMaze.h"
#include "Moveable.h"
#include "Pacman.h"
#include "Ghost.h"

class VulkanAPIHandler;

enum DescriptorLayoutType {
	DESC_LAYOUT_RENDERABLE = 0,
	DESC_LAYOUT_SCENE
};

enum RenderableTypes {
	RENDERABLE_MAZE = 0,
	RENDERABLE_PACMAN = 1,
	RENDERABLE_GHOST = 2
};

class Scene {
public:
	Scene(VulkanAPIHandler* vulkanAPI);
	
	void updateUniformBuffers(glm::mat4 projectionMatrix, glm::mat4 viewMatrix);
	void update(float deltaTime);
	void handleInput(GLFWKeyEvent event);
	
	void createTextureImages();
	void createTextureImageViews();
	void createTextureSamplers();
	void createVertexIndexBuffers();
	void createUniformBuffers();
	void createDescriptorSetLayouts();
	void createDescriptorSets(VkDescriptorPool descPool);
	void createRenderables();
	
	std::vector<std::pair<RenderableTypes, std::shared_ptr<Renderable>>> getRenderableObjects();
	VkDescriptorSetLayout getDescriptorSetLayout(DescriptorLayoutType type);
	VkDescriptorSet getDescriptorSet();
private:
	VulkanAPIHandler* vulkanAPIHandler;
	VkDescriptorSet descriptorSet;

	std::vector<std::pair<RenderableTypes, std::shared_ptr<Renderable>>> renderableObjects{};
	SceneUBO sceneUBO;

	VDeleter<VkDevice> device;
	VDeleter<VkDescriptorSetLayout> descriptorSetLayout{ device, vkDestroyDescriptorSetLayout };

	VDeleter<VkBuffer> uniformStagingBuffer{ device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> uniformStagingBufferMemory{ device, vkFreeMemory };
	VDeleter<VkBuffer> uniformBuffer{ device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> uniformBufferMemory{ device, vkFreeMemory };

	int mazeIndex{0};
	int pacmanIndex{1};
	std::vector<glm::vec4> spawnPositions{
		glm::vec4(350, 30, 400, 1),
		glm::vec4(100, 50, 100, 1),
		glm::vec4(700, 50, 100, 1),
		glm::vec4(700, 50, 700, 1)
	};
};

