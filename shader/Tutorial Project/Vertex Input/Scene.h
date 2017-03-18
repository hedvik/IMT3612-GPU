#pragma once
#include <memory>
#include <vector>
#include <glm\glm.hpp>
#include "Renderable.h"

class VulkanAPIHandler;

enum DescriptorLayoutType {
	DESC_LAYOUT_RENDERABLE = 0,
	DESC_LAYOUT_SCENE
};

class Scene {
public:
	Scene(VulkanAPIHandler* vulkanAPI);
	std::vector<std::shared_ptr<Renderable>> getRenderableObjects();
	void updateUniformBuffers(glm::mat4 projectionMatrix, glm::mat4 viewMatrix);
	void createTextureImages();
	void createTextureImageViews();
	void createTextureSamplers();
	void createVertexIndexBuffers();
	void createUniformBuffers();
	void createDescriptorSetLayouts();
	void createDescriptorSets(VkDescriptorPool descPool);
	void createRenderables();
	VkDescriptorSetLayout getDescriptorSetLayout(DescriptorLayoutType type);
	VkDescriptorSet getDescriptorSet();
private:
	VulkanAPIHandler* vulkanAPIHandler;
	VkDescriptorSet descriptorSet;

	std::vector<std::shared_ptr<Renderable>> renderableObjects{};
	SceneUBO sceneUBO;

	VDeleter<VkDevice> device;
	VDeleter<VkDescriptorSetLayout> descriptorSetLayout{ device, vkDestroyDescriptorSetLayout };

	VDeleter<VkBuffer> uniformStagingBuffer{ device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> uniformStagingBufferMemory{ device, vkFreeMemory };
	VDeleter<VkBuffer> uniformBuffer{ device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> uniformBufferMemory{ device, vkFreeMemory };
};

