#include "Scene.h"
#include "VulkanAPIHandler.h"

Scene::Scene(VulkanAPIHandler* vulkanAPI) {
	vulkanAPIHandler = vulkanAPI;
	device = vulkanAPIHandler->getDevice();

	sceneUBO.lightPositions.push_back(glm::vec4(0, 4, 0, 1));
}

std::vector<std::shared_ptr<Renderable>> Scene::getRenderableObjects() {
	return renderableObjects;
}

void Scene::updateUniformBuffers(glm::mat4 projectionMatrix, glm::mat4 viewMatrix) {
	for (auto& renderable : renderableObjects) {
		renderable->updateUniformBuffer(projectionMatrix, viewMatrix);
	}
}

void Scene::createTextureImages() {
	for (auto& renderable : renderableObjects) {
		renderable->createTextureImage();
	}
}

void Scene::createTextureImageViews() {
	for (auto& renderable : renderableObjects) {
		renderable->createTextureImageView();
	}
}

void Scene::createTextureSamplers() {
	for (auto& renderable : renderableObjects) {
		renderable->createTextureSampler();
	}
}

void Scene::createVertexIndexBuffers() {
	for (auto& renderable : renderableObjects) {
		renderable->createVertexIndexBuffers();
	}
}

void Scene::createUniformBuffers() {
	for (auto& renderable : renderableObjects) {
		renderable->createUniformBuffer();
	}

	VkDeviceSize bufferSize = sizeof(RenderableUBO);

	vulkanAPIHandler->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformStagingBuffer, uniformStagingBufferMemory);
	vulkanAPIHandler->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, uniformBuffer, uniformBufferMemory);
}

void Scene::createDescriptorSetLayouts() {
	for (auto& renderable : renderableObjects) {
		renderable->createDescriptorSetLayout();
	}
	
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 2;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding binding = uboLayoutBinding;
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &binding;

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, descriptorSetLayout.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	} 
}

void Scene::createDescriptorSets(VkDescriptorPool descPool) {
	for (auto& renderable : renderableObjects) {
		renderable->createDescriptorSet(descPool);
	}
	
	// Creating the descriptor set for the scene UBO
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor set!");
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(sceneUBO);

	VkWriteDescriptorSet descriptorWrites = {};

	descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites.dstSet = descriptorSet;
	descriptorWrites.dstBinding = 2;
	descriptorWrites.dstArrayElement = 0;
	descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites.descriptorCount = 1;
	descriptorWrites.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(device, 1, &descriptorWrites, 0, nullptr); 
}

void Scene::createRenderables() {
	// This is where we initialise all of the renderables
	renderableObjects.emplace_back(new Renderable(vulkanAPIHandler, glm::vec3(-1, 0, 0), COEURL_TEXTURE_PATH, CUBE_MODEL_PATH));
	renderableObjects.emplace_back(new Renderable(vulkanAPIHandler, glm::vec3(1, 0, 0), DEFAULT_TEXTURE_PATH, CUBE_MODEL_PATH, glm::vec3(0.5, 0, 0.5)));
}

VkDescriptorSetLayout Scene::getDescriptorSetLayout(DescriptorLayoutType type) {
	if (type == DESC_LAYOUT_RENDERABLE) {
		return renderableObjects[0]->getDescriptorLayout();
	} 
	else if (type == DESC_LAYOUT_SCENE) {
		return descriptorSetLayout;
	} 
}

VkDescriptorSet Scene::getDescriptorSet() {
	return descriptorSet;
}

