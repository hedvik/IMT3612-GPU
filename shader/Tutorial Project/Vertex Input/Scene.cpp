#include "Scene.h"

Scene::Scene() {
}

std::vector<std::shared_ptr<Renderable>> Scene::getRenderableObjects() {
	return renderableObjects;
}

void Scene::loadModel(std::string path, glm::vec3 positionOffset, glm::vec3 color, int index) {
	renderableObjects[index]->loadModel(path, positionOffset, color);
}

void Scene::updateUniformBuffers() {
	for (auto& renderable : renderableObjects) {
		renderable->updateUniformBuffer();
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
}

void Scene::createDescriptorSets(VkDescriptorSetLayout descSetLayout, VkDescriptorPool descPool) {
	for (auto& renderable : renderableObjects) {
		renderable->createDescriptorSet(descSetLayout, descPool);
	}
}

void Scene::createRenderable(VulkanAPIHandler * vulkanAPI) {
	renderableObjects.emplace_back(new Renderable(vulkanAPI));
}

