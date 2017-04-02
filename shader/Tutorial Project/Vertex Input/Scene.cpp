#include "Scene.h"
#include "VulkanAPIHandler.h"

Scene::Scene(VulkanAPIHandler* vulkanAPI) {
	vulkanAPIHandler = vulkanAPI;
	device = vulkanAPIHandler->getDevice();

	sceneUBO.lightPositions[0] = glm::vec4(400, 100, 400, 1.0);
	sceneUBO.lightColors[0] = glm::vec4(1, 1, 1, 1);
}

std::vector<std::pair<RenderableTypes, std::shared_ptr<Renderable>>> Scene::getRenderableObjects() {
	return renderableObjects;
}

void Scene::updateUniformBuffers(glm::mat4 projectionMatrix, glm::mat4 viewMatrix) {
	for (auto& renderable : renderableObjects) {
		renderable.second->updateUniformBuffer(projectionMatrix, viewMatrix);
	}

	void* data;
	vkMapMemory(device, uniformStagingBufferMemory, 0, sizeof(sceneUBO), 0, &data);
	memcpy(data, &sceneUBO, sizeof(sceneUBO));
	vkUnmapMemory(device, uniformStagingBufferMemory);

	vulkanAPIHandler->copyBuffer(uniformStagingBuffer, uniformBuffer, sizeof(sceneUBO));
}

void Scene::update(float deltaTime) {
	for (auto& renderable : renderableObjects) {
		renderable.second->update(deltaTime);
	}

	// For each ghost, check if it collides with pacman. There is probably a better way of doing this
	for (auto& ghost : renderableObjects) {
		if (ghost.first == RENDERABLE_GHOST) {
			if (CollisionHandler::checkCollision(
					std::dynamic_pointer_cast<Ghost>(ghost.second)->getCollisionRect(), 
					std::dynamic_pointer_cast<Pacman>(renderableObjects[pacmanIndex].second)->getCollisionRect())) {
				std::random_device rd;
				std::mt19937 gen(rd());
				std::uniform_int_distribution<> dis(0, 3);

				std::dynamic_pointer_cast<Ghost>(ghost.second)->respawn(spawnPositions[dis(gen)]);
			}
		}
	}

}

void Scene::handleInput(GLFWKeyEvent event) {
	// Another alternative would be to just make handleInput virtual. Might be better if you want to control ghosts in some way too
	std::dynamic_pointer_cast<Pacman>(renderableObjects[pacmanIndex].second)->handleInput(event);
}

void Scene::createTextureImages() {
	for (auto& renderable : renderableObjects) {
		renderable.second->createTextureImage();
	}
}

void Scene::createTextureImageViews() {
	for (auto& renderable : renderableObjects) {
		renderable.second->createTextureImageView();
	}
}

void Scene::createTextureSamplers() {
	for (auto& renderable : renderableObjects) {
		renderable.second->createTextureSampler();
	}
}

void Scene::createVertexIndexBuffers() {
	for (auto& renderable : renderableObjects) {
		renderable.second->createVertexIndexBuffers();
	}
}

void Scene::createUniformBuffers() {
	for (auto& renderable : renderableObjects) {
		renderable.second->createUniformBuffers();
	}

	VkDeviceSize bufferSize = sizeof(SceneUBO);

	vulkanAPIHandler->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformStagingBuffer, uniformStagingBufferMemory);
	vulkanAPIHandler->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, uniformBuffer, uniformBufferMemory);
}

void Scene::createDescriptorSetLayouts() {
	for (auto& renderable : renderableObjects) {
		renderable.second->createDescriptorSetLayout();
	}
	
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;

	VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, descriptorSetLayout.replace());
	if (result != VK_SUCCESS) {
		std::cout << result << std::endl;
		throw std::runtime_error("failed to create descriptor set layout!");
	} 
}

void Scene::createDescriptorSets(VkDescriptorPool descPool) {
	for (auto& renderable : renderableObjects) {
		renderable.second->createDescriptorSet(descPool);
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
	bufferInfo.range = sizeof(SceneUBO);

	VkWriteDescriptorSet descriptorWrites = {};

	descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites.dstSet = descriptorSet;
	descriptorWrites.dstBinding = 0;
	descriptorWrites.dstArrayElement = 0;
	descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites.descriptorCount = 1;
	descriptorWrites.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(device, 1, &descriptorWrites, 0, nullptr); 
}

void Scene::createRenderables() {
	// This is where we initialise all of the renderables
	renderableObjects.emplace_back(std::make_pair(RENDERABLE_MAZE, std::make_shared<RenderableMaze>(vulkanAPIHandler, glm::vec4(0, 0, 0, 1))));
	
	renderableObjects.emplace_back(
		std::make_pair(RENDERABLE_PACMAN, 
		std::make_shared<Pacman>(
			std::dynamic_pointer_cast<RenderableMaze>(renderableObjects[mazeIndex].second), 
			vulkanAPIHandler, 
			spawnPositions[0], 
			DEFAULT_TEXTURE_PATH, 
			SPHERE_MODEL_PATH, 
			glm::vec3(30, 30, 30), 
			glm::vec4(1, 1, 0, 1))));
	
	
	renderableObjects.emplace_back(
		std::make_pair(RENDERABLE_GHOST, 
		std::make_shared<Ghost>(
			&sceneUBO, 
			std::dynamic_pointer_cast<Pacman>(renderableObjects[pacmanIndex].second),
			std::dynamic_pointer_cast<RenderableMaze>(renderableObjects[mazeIndex].second), 
			vulkanAPIHandler, 
			1, 
			spawnPositions[1], 
			glm::vec3(30, 30, 30), 
			glm::vec4(1, 0, 0, 1))));

	renderableObjects.emplace_back(
		std::make_pair(RENDERABLE_GHOST, 
		std::make_shared<Ghost>(
			&sceneUBO,
			std::dynamic_pointer_cast<Pacman>(renderableObjects[pacmanIndex].second),
			std::dynamic_pointer_cast<RenderableMaze>(renderableObjects[mazeIndex].second),
			vulkanAPIHandler,
			2,
			spawnPositions[2],
			glm::vec3(30, 30, 30),
			glm::vec4(0, 1, 0, 1))));

	renderableObjects.emplace_back(
		std::make_pair(RENDERABLE_GHOST, 
		std::make_shared<Ghost>(
			&sceneUBO,
			std::dynamic_pointer_cast<Pacman>(renderableObjects[pacmanIndex].second),
			std::dynamic_pointer_cast<RenderableMaze>(renderableObjects[mazeIndex].second),
			vulkanAPIHandler,
			3,
			spawnPositions[3],
			glm::vec3(30, 30, 30),
			glm::vec4(0, 0, 1, 1))));
}

VkDescriptorSetLayout Scene::getDescriptorSetLayout(DescriptorLayoutType type) {
	if (type == DESC_LAYOUT_RENDERABLE) {
		return renderableObjects[0].second->getDescriptorLayout();
	} 
	else if (type == DESC_LAYOUT_SCENE) {
		return descriptorSetLayout;
	} 
	else {
		return descriptorSetLayout;
	}
}

VkDescriptorSet Scene::getDescriptorSet() {
	return descriptorSet;
}

