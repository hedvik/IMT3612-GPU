#pragma once
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <memory>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ShaderTypes.h"
#include "VDeleter.h"

class VulkanAPIHandler;

class Renderable {
public:
	Renderable(VulkanAPIHandler* vkAPIHandler);
	~Renderable();
	
	glm::mat4 generateMVP(glm::mat4 projectionMatrix, glm::mat4 viewMatrix);
	void createVertexIndexBuffers();
	void createUniformBuffer();
	VkBuffer getVertexBuffer();
	VkBuffer getIndexBuffer();
	int numIndices();
	void loadModel(std::string path, glm::vec3 positionOffset, glm::vec3 color);
	void createTextureImage();
	void createTextureImageView();
	void createTextureSampler();
	void createDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool);
	VkDescriptorSet getDescriptorSet();
	void updateUniformBuffer();
protected:
	std::vector<Vertex> vertices{};
	std::vector<uint32_t> indices{};
	glm::mat4 modelMatrix{1.f};
	glm::vec3 position{};
private:
	VulkanAPIHandler* vulkanAPIHandler;
	VDeleter<VkDevice> device;

	VDeleter<VkImage> textureImage{ device , vkDestroyImage };
	VDeleter<VkImageView> textureImageView{ device, vkDestroyImageView };
	VDeleter<VkSampler> textureSampler{ device, vkDestroySampler };
	VDeleter<VkDeviceMemory> textureImageMemory{ device, vkFreeMemory };

	VDeleter<VkBuffer> vertexBuffer{device, vkDestroyBuffer};
	VDeleter<VkDeviceMemory> vertexBufferMemory{ device, vkFreeMemory };
	VDeleter<VkBuffer> indexBuffer{ device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> indexBufferMemory{ device, vkFreeMemory };
	
	VDeleter<VkBuffer> uniformStagingBuffer{ device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> uniformStagingBufferMemory{ device, vkFreeMemory };
	VDeleter<VkBuffer> uniformBuffer{ device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> uniformBufferMemory{ device, vkFreeMemory };

	VkDescriptorSet descriptorSet;
};

