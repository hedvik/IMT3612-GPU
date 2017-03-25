#pragma once
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <memory>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "Structs.h"
#include "VDeleter.h"

class VulkanAPIHandler;

class Renderable {
public:
	Renderable();
	Renderable(
		VulkanAPIHandler* vkAPIHandler, 
		glm::vec4 pos, 
		std::string texPath, 
		std::string meshPath, 
		glm::vec3 renderableScale = glm::vec3(1.f, 1.f, 1.f),
		glm::vec4 color = glm::vec4(1, 1, 1, 1), 
		bool invertedNormals = false);
	~Renderable();

	int numIndices();
	void updateUniformBuffer(glm::mat4 projectionMatrix, glm::mat4 viewMatrix);

	VkBuffer getVertexBuffer();
	VkBuffer getIndexBuffer();
	VkDescriptorSet getDescriptorSet();
	VkDescriptorSetLayout getDescriptorLayout();

	void createVertexIndexBuffers();
	void createUniformBuffer();
	void createTextureImage();
	void createTextureImageView();
	void createTextureSampler();
	void createDescriptorSetLayout();
	void createDescriptorSet(VkDescriptorPool descriptorPool);
protected:
	Renderable(VulkanAPIHandler* vkAPIHandler, glm::vec4 pos, std::string texPath);

	glm::vec3 position{};
	glm::vec3 scale{1.f, 1.f, 1.f};
	std::vector<Vertex> vertices{};
	std::vector<uint32_t> indices{};
	glm::mat4 modelMatrix{1.f};
	glm::vec4 baseColor{};
private:
	VulkanAPIHandler* vulkanAPIHandler;
	VDeleter<VkDevice> device;
	VkDescriptorSet descriptorSet;

	std::string texturePath{};
	std::string modelPath{};

	VDeleter<VkDescriptorSetLayout> descriptorSetLayout{ device, vkDestroyDescriptorSetLayout };

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

	void loadModel(bool invertNormals);
};

