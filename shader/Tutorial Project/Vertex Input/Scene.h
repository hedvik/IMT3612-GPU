#pragma once
#define _USE_MATH_DEFINES
#include <memory>
#include <vector>
#include <random>
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

class Scene {
public:
	Scene(VulkanAPIHandler* vulkanAPI);
	~Scene();

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
	void prepareCubeMaps();
	void prepareOffscreenFramebuffer();
	void updateCubeFace(uint32_t faceIndex, uint32_t lightIndex);
	void buildOffscreenCommandBuffer();
	void createOffscreenPipelineLayout();
	void prepareOffscreenRenderpass();
	void prepareOffscreenPipeline(VkGraphicsPipelineCreateInfo pipelineInfo);

	VkSemaphore getOffscreenSemaphore();
	VkCommandBuffer getOffscreenCommandBuffer();
	std::vector<std::pair<RenderableInformation, std::shared_ptr<Renderable>>> getRenderableObjects();
	VkDescriptorSetLayout getDescriptorSetLayout(DescriptorLayoutType type);
	VkDescriptorSet getDescriptorSet();
private:
	VulkanAPIHandler* vulkanAPIHandler;
	VkDescriptorSet descriptorSet;

	std::vector<std::pair<RenderableInformation, std::shared_ptr<Renderable>>> renderableObjects{};
	std::shared_ptr<RenderableMaze> maze;
	std::shared_ptr<Pacman> pacman;
	std::vector<std::shared_ptr<Ghost>> ghosts;

	SceneUBO sceneUBO;

	OffscreenPass offscreenPass;
	VkFormat frameBufferDepthFormat;

	VDeleter<VkDevice> device;
	VDeleter<VkDescriptorSetLayout> descriptorSetLayout{ device, vkDestroyDescriptorSetLayout };

	std::vector<VDeleter<VkImage>> shadowCubeMapImages;
	std::vector<VDeleter<VkImageView>> shadowCubeMapImageViews;
	std::vector<VDeleter<VkSampler>> shadowCubeMapSamplers;
	std::vector<VDeleter<VkDeviceMemory>> shadowCubeMapMemories;

	VDeleter<VkBuffer> uniformStagingBuffer{ device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> uniformStagingBufferMemory{ device, vkFreeMemory };
	VDeleter<VkBuffer> uniformBuffer{ device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> uniformBufferMemory{ device, vkFreeMemory };

	VDeleter<VkPipelineLayout> offscreenPipelineLayout{ device, vkDestroyPipelineLayout };
	VDeleter<VkPipeline> offscreenPipeline{ device, vkDestroyPipeline };

	std::vector<glm::vec4> spawnPositions {
		glm::vec4(350.f, 30.f, 400.f, 1.f),
		glm::vec4(100.f, 30.f, 100.f, 1.f),
		glm::vec4(700.f, 30.f, 100.f, 1.f),
		glm::vec4(700.f, 30.f, 700.f, 1.f)
	};

	std::vector<glm::vec4> ghostColors{
		glm::vec4(1.f, 0.f, 0.f, 1.f),
		glm::vec4(0.f, 1.f, 0.f, 1.f),
		glm::vec4(0.f, 0.f, 1.f, 1.f)
	};
};