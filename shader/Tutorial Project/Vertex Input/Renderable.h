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
		bool invertedNormals = false);
	Renderable(
		VulkanAPIHandler* vkAPIHandler,
		glm::vec4 pos,
		std::string texPath,
		std::string meshPath,
		glm::vec3 renderableScale,
		glm::vec4 c,
		bool invertedNormals = false);
	Renderable(
		VulkanAPIHandler* vkAPIHandler,
		glm::vec3 renderableScale,
		glm::vec4 c,
		bool invertedNormals = false);
	~Renderable();

	virtual void update(float deltaTime);

	int numIndices();
	glm::vec3 getPosition();

	void updateUniformBuffer(glm::mat4 projectionMatrix, glm::mat4 viewMatrix);

	VkBuffer getVertexBuffer();
	VkBuffer getIndexBuffer();
	VkDescriptorSet getDescriptorSet();
	VkDescriptorSetLayout getDescriptorLayout();

	void createVertexIndexBuffers();
	void createUniformBuffers();
	void createTextureImage();
	void createTextureImageView();
	void createTextureSampler();
	void createDescriptorSetLayout();
	void createDescriptorSet(VkDescriptorPool descriptorPool);
protected:
	Renderable(VulkanAPIHandler* vkAPIHandler, glm::vec4 pos, std::string texturePath);

	VulkanAPIHandler* vulkanAPIHandler;
	
	std::vector<Vertex> vertices{};
	std::vector<uint32_t> indices{};
	glm::mat4 modelMatrix{1.f};
	
	glm::vec3 position{0.f, 0.f, 0.f};
	glm::vec3 scale{1.f, 1.f, 1.f};
	glm::vec4 baseColor{1.f, 1.f, 1.f, 1.f};

	RenderableMaterial material{};
private:
	VDeleter<VkDevice> device;
	VkDescriptorSet descriptorSet;

	std::string texturePath{DEFAULT_TEXTURE_PATH};
	std::string modelPath{CUBE_MODEL_PATH};

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

	VDeleter<VkBuffer> materialBuffer{ device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> materialBufferMemory{ device, vkFreeMemory };

	void loadModel(bool invertNormals);
};

