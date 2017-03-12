#pragma once
#include <memory>
#include <vector>
#include <glm\glm.hpp>
#include "Renderable.h"

class VulkanAPIHandler;

class Scene {
public:
	Scene();
	std::vector<std::shared_ptr<Renderable>> getRenderableObjects();
	void loadModel(std::string path, glm::vec3 positionOffset = glm::vec3(0, 0, 0), glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f), int index = 0);
	void updateUniformBuffers();
	void createTextureImages();
	void createTextureImageViews();
	void createTextureSamplers();
	void createVertexIndexBuffers();
	void createUniformBuffers();
	void createDescriptorSets(VkDescriptorSetLayout descSetLayout, VkDescriptorPool descPool);
	void createRenderable(VulkanAPIHandler* vulkanAPI);
private:
	std::vector<std::shared_ptr<Renderable>> renderableObjects{};
};

