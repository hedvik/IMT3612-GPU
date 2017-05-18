#include "Scene.h"
#include "VulkanAPIHandler.h"

Scene::Scene(VulkanAPIHandler* vulkanAPI) {
	vulkanAPIHandler = vulkanAPI;
	device = vulkanAPIHandler->getDevice();

	sceneUBO.lightPositions[0] = glm::vec4(410, 100, 410, 1.0);
	sceneUBO.lightPositions[1] = spawnPositions[1];
	sceneUBO.lightPositions[2] = spawnPositions[2];
	sceneUBO.lightPositions[3] = spawnPositions[3];
	sceneUBO.lightColors[0] = glm::vec4(1, 1, 1, 1);

	frameBufferDepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

	// Offscreen matrix
	// The projectionMatrix HAS to have a fov of M_PI / 2  (90 degrees). Any other FoV value distorts the map
	sceneUBO.projectionMatrix = glm::perspective(glm::radians(90.0f), 1.0f, Z_NEAR, Z_FAR);
	sceneUBO.projectionMatrix[1][1] *= -1;

	for (int i = 0; i < NUM_LIGHTS; i++) {
		shadowCubeMapImages.emplace_back(VDeleter<VkImage>{ device, vkDestroyImage });
		shadowCubeMapImageViews.emplace_back(VDeleter<VkImageView>{ device, vkDestroyImageView });
		shadowCubeMapSamplers.emplace_back(VDeleter<VkSampler>{ device, vkDestroySampler });
		shadowCubeMapMemories.emplace_back(VDeleter<VkDeviceMemory>{ device, vkFreeMemory });
	}
}

Scene::~Scene() {
	// Cleaning up the framebuffer attachment
	// Color attachment
	vkDestroyImageView(device, offscreenPass.color.view, nullptr);
	vkDestroyImage(device, offscreenPass.color.image, nullptr);
	vkFreeMemory(device, offscreenPass.color.memory, nullptr);

	// Depth attachment
	vkDestroyImageView(device, offscreenPass.depth.view, nullptr);
	vkDestroyImage(device, offscreenPass.depth.image, nullptr);
	vkFreeMemory(device, offscreenPass.depth.memory, nullptr);

	// Cleaning up the framebuffer, renderpass and semaphore
	vkDestroyFramebuffer(device, offscreenPass.frameBuffer, nullptr);
	vkDestroyRenderPass(device, offscreenPass.renderPass, nullptr);

	vkDestroySemaphore(device, offscreenPass.semaphore, nullptr);
}

VkSemaphore Scene::getOffscreenSemaphore() {
	return offscreenPass.semaphore;
}

VkCommandBuffer Scene::getOffscreenCommandBuffer() {
	return offscreenPass.commandBuffer;
}

std::vector<std::pair<RenderableInformation, std::shared_ptr<Renderable>>> Scene::getRenderableObjects() {
	return renderableObjects;
}

void Scene::updateUniformBuffers(glm::mat4 projectionMatrix, glm::mat4 viewMatrix) {
	for (auto& renderable : renderableObjects) {
		renderable.second->updateUniformBuffer(projectionMatrix, viewMatrix);
	}

	for (int i = 0; i < shadowCubeMapImages.size(); i++) {
		sceneUBO.lightOffsetMatrices[i] = glm::translate(glm::mat4(1.f), glm::vec3(-sceneUBO.lightPositions[i].x, -sceneUBO.lightPositions[i].y, -sceneUBO.lightPositions[i].z));
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

	// For each ghost, check if it collides with pacman. 
	for (auto& ghost : ghosts) {
		if (CollisionHandler::checkCollision(ghost->getCollisionRect(),  pacman->getCollisionRect())) {
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> dis(0, 3);

			ghost->respawn(spawnPositions[dis(gen)]);
		}
	}
}

void Scene::handleInput(GLFWKeyEvent event) {
	pacman->handleInput(event);
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

	vulkanAPIHandler->createBuffer(bufferSize, 
								   VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
								   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
								   uniformStagingBuffer, 
								   uniformStagingBufferMemory);

	vulkanAPIHandler->createBuffer(bufferSize, 
								   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
								   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
								   uniformBuffer, 
								   uniformBufferMemory);
}

void Scene::createDescriptorSetLayouts() {
	for (auto& renderable : renderableObjects) {
		renderable.second->createDescriptorSetLayout();
	}
	
	// Scene descriptor set layout
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding cubeMapLayoutBinding = {};
	cubeMapLayoutBinding.binding = 1;
	cubeMapLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	cubeMapLayoutBinding.descriptorCount = shadowCubeMapImages.size();
	cubeMapLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	
	std::array<VkDescriptorSetLayoutBinding, 2> layoutBindings = { uboLayoutBinding, cubeMapLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = layoutBindings.size();
	layoutInfo.pBindings = layoutBindings.data();

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
	
	// Creating the descriptor set for the scene UBO and shadow cube map
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

	std::array<VkDescriptorImageInfo, NUM_LIGHTS> cubeMapInfo = {};
	cubeMapInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	cubeMapInfo[0].imageView = shadowCubeMapImageViews[0];
	cubeMapInfo[0].sampler = shadowCubeMapSamplers[0];
	
	for (int i = 1; i < shadowCubeMapImages.size(); i++) {
		cubeMapInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		cubeMapInfo[i].imageView = shadowCubeMapImageViews[i];
		cubeMapInfo[i].sampler = shadowCubeMapSamplers[i];
	} 

	std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = cubeMapInfo.size();
	descriptorWrites[1].pImageInfo = cubeMapInfo.data();

	vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr); 
}

void Scene::createRenderables() {
	// This is where we initialise all of the renderables
	maze = std::make_shared<RenderableMaze>(vulkanAPIHandler, glm::vec4(0, 0, 0, 1));
	pacman = std::make_shared<Pacman>(maze, vulkanAPIHandler, spawnPositions[0], glm::vec3(30, 30, 30), glm::vec4(1, 1, 0, 1));
	
	for (int i = 0; i < NUM_GHOSTS; i++) {
		ghosts.emplace_back(std::make_shared<Ghost>(&sceneUBO, pacman, maze, vulkanAPIHandler, i + 1, spawnPositions[i + 1], glm::vec3(30, 30, 30), ghostColors[i]));
	}

	renderableObjects.emplace_back(std::make_pair<RenderableInformation, std::shared_ptr<Renderable>>(RenderableInformation(RENDERABLE_MAZE), maze));
	renderableObjects.emplace_back(std::make_pair<RenderableInformation, std::shared_ptr<Renderable>>(RenderableInformation(RENDERABLE_PACMAN), pacman));

	for (auto& ghost : ghosts) {
		renderableObjects.emplace_back(std::make_pair<RenderableInformation, std::shared_ptr<Renderable>>(RenderableInformation(RENDERABLE_GHOST, false), ghost));
	}
}

// Based on https://github.com/SaschaWillems/Vulkan/blob/master/shadowmappingomni/shadowmappingomni.cpp
void Scene::prepareCubeMaps() {
	// 32 bit float format for higher precision
	VkFormat format = OFFSCREEN_FB_COLOR_FORMAT;

	// Cube map image description
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = format;
	imageCreateInfo.extent = { CUBE_MAP_TEX_DIM, CUBE_MAP_TEX_DIM, 1 };
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = NUM_CUBE_FACES;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	
	// Setting up and recording the command buffer
	VkCommandBuffer layoutCmd = vulkanAPIHandler->beginSingleTimeCommands();

	// Create cube map images
	for (int i = 0; i < shadowCubeMapImages.size(); i++) {
		vulkanAPIHandler->createImage(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shadowCubeMapImages[i], shadowCubeMapMemories[i]);
	}

	// Image barrier for optimal image (target). The cube map has 6 array layers, one for each face
	for (int i = 0; i < shadowCubeMapImages.size(); i++) {
		vulkanAPIHandler->transitionImageLayout(layoutCmd, shadowCubeMapImages[i], format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, NUM_CUBE_FACES);
	}

	// Flush command buffer
	vulkanAPIHandler->endSingleTimeCommands(layoutCmd);

	// Create samplers
	VkSamplerCreateInfo sampler = {};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.magFilter = CUBE_MAP_TEX_FILTER;
	sampler.minFilter = CUBE_MAP_TEX_FILTER;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	sampler.addressModeV = sampler.addressModeU;
	sampler.addressModeW = sampler.addressModeU;
	sampler.mipLodBias = 0.0f;
	sampler.maxAnisotropy = 16.f;
	sampler.anisotropyEnable = VK_TRUE;
	sampler.compareOp = VK_COMPARE_OP_NEVER;
	sampler.minLod = 0.0f;
	sampler.maxLod = 1.0f;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	
	for (int i = 0; i < shadowCubeMapImages.size(); i++) {
		if (vkCreateSampler(device, &sampler, nullptr, shadowCubeMapSamplers[i].replace()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create cubemap sampler");
		}
	}

	// Create image views
	for (int i = 0; i < shadowCubeMapImages.size(); i++) {
		VkImageViewCreateInfo view = {};
		view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		view.format = format;
		view.components = { VK_COMPONENT_SWIZZLE_R };
		view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		view.subresourceRange.layerCount = NUM_CUBE_FACES;
		view.image = shadowCubeMapImages[i];

		vulkanAPIHandler->createImageView(view, shadowCubeMapImageViews[i]);
	}
}

// Prepare a new framebuffer for offscreen rendering
// The contents of this framebuffer are then
// copied to the different cube map faces
void Scene::prepareOffscreenFramebuffer() {
	offscreenPass.width = OFFSCREEN_FB_TEX_DIM;
	offscreenPass.height = OFFSCREEN_FB_TEX_DIM;

	// Color attachment
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = OFFSCREEN_FB_COLOR_FORMAT;
	imageCreateInfo.extent.width = offscreenPass.width;
	imageCreateInfo.extent.height = offscreenPass.height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	// Image of the framebuffer is blit source
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkImageViewCreateInfo colorImageView = {};
	colorImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	colorImageView.format = OFFSCREEN_FB_COLOR_FORMAT;
	colorImageView.flags = 0;
	colorImageView.subresourceRange = {};
	colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	colorImageView.subresourceRange.baseMipLevel = 0;
	colorImageView.subresourceRange.levelCount = 1;
	colorImageView.subresourceRange.baseArrayLayer = 0;
	colorImageView.subresourceRange.layerCount = 1;

	vulkanAPIHandler->createImage(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, offscreenPass.color.image, offscreenPass.color.memory);
	
	// Start recording command buffer
	VkCommandBuffer layoutCmd = vulkanAPIHandler->beginSingleTimeCommands();

	vulkanAPIHandler->transitionImageLayout(layoutCmd, offscreenPass.color.image, OFFSCREEN_FB_COLOR_FORMAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	colorImageView.image = offscreenPass.color.image;

	vulkanAPIHandler->createImageView(colorImageView, offscreenPass.color.view);

	// Depth stencil attachment
	imageCreateInfo.format = frameBufferDepthFormat;
	imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	VkImageViewCreateInfo depthStencilView = {};
	depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthStencilView.format = frameBufferDepthFormat;
	depthStencilView.flags = 0;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = 1;

	vulkanAPIHandler->createImage(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, offscreenPass.depth.image, offscreenPass.depth.memory);

	vulkanAPIHandler->transitionImageLayout(layoutCmd, offscreenPass.depth.image, frameBufferDepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, true);

	// Stop commandBuffer recording
	vulkanAPIHandler->endSingleTimeCommands(layoutCmd);

	depthStencilView.image = offscreenPass.depth.image;
	vulkanAPIHandler->createImageView(depthStencilView, offscreenPass.depth.view);

	VkImageView attachments[2];
	attachments[0] = offscreenPass.color.view;
	attachments[1] = offscreenPass.depth.view;

	VkFramebufferCreateInfo fbufCreateInfo = {};
	fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbufCreateInfo.renderPass = offscreenPass.renderPass;
	fbufCreateInfo.attachmentCount = 2;
	fbufCreateInfo.pAttachments = attachments;
	fbufCreateInfo.width = offscreenPass.width;
	fbufCreateInfo.height = offscreenPass.height;
	fbufCreateInfo.layers = 1;

	if (vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &offscreenPass.frameBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create offscreen framebuffer");
	}
}

// Updates a single cube map face
// Renders the scene with face's view and does 
// a copy from framebuffer to cube face
// Uses push constants for quick update of
// view matrix for the current cube map face
void Scene::updateCubeFace(uint32_t faceIndex, uint32_t lightIndex) {
	VkClearValue clearValues[2];
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	// Reuse render pass from example pass
	renderPassBeginInfo.renderPass = offscreenPass.renderPass;
	renderPassBeginInfo.framebuffer = offscreenPass.frameBuffer;
	renderPassBeginInfo.renderArea.extent.width = offscreenPass.width;
	renderPassBeginInfo.renderArea.extent.height = offscreenPass.height;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	// Update view matrix via push constant
	glm::mat4 viewMatrix = glm::mat4();
	glm::vec3 lightPosition = glm::vec3(0, 0, 0);
	
	// Cube map faces generally have to use -y as their up axis. http://stackoverflow.com/questions/11685608/convention-of-faces-in-opengl-cubemapping
	// The math is also inverted due to this.
	switch (faceIndex) {
	case 0: // POSITIVE_X 
		viewMatrix = glm::lookAt(lightPosition, lightPosition - glm::vec3(1, 0, 0), glm::vec3(0, -1, 0));
		break;
	case 1:	// NEGATIVE_X
		viewMatrix = glm::lookAt(lightPosition, lightPosition + glm::vec3(1, 0, 0), glm::vec3(0, -1, 0));
		break;
	case 2:	// POSITIVE_Y
		viewMatrix = glm::lookAt(lightPosition, lightPosition - glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
		break;
	case 3:	// NEGATIVE_Y
		viewMatrix = glm::lookAt(lightPosition, lightPosition + glm::vec3(0, 1, 0), glm::vec3(0, 0, -1));
		break;
	case 4:	// POSITIVE_Z
		viewMatrix = glm::lookAt(lightPosition, lightPosition - glm::vec3(0, 0, 1), glm::vec3(0, -1, 0));
		break;
	case 5:	// NEGATIVE_Z
		viewMatrix = glm::lookAt(lightPosition, lightPosition + glm::vec3(0, 0, 1), glm::vec3(0, -1, 0));
		break;
	} 

	// Render scene from cube face's point of view
	vkCmdBeginRenderPass(offscreenPass.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	PushConstants pushConstant(viewMatrix, lightIndex);

	// Update shader push constant block
	// Contains current face view matrix
	vkCmdPushConstants(offscreenPass.commandBuffer,
					   offscreenPipelineLayout,
					   VK_SHADER_STAGE_VERTEX_BIT,
					   0,
					   sizeof(PushConstants),
					   &pushConstant);

	// Binding buffers and issuing draw calls per renderable
	vkCmdBindPipeline(offscreenPass.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenPipeline);
	vkCmdBindDescriptorSets(offscreenPass.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenPipelineLayout, SCENE_UBO, 1, &descriptorSet, 0, nullptr);
	
	VkDeviceSize offsets[] = { 0 };
	for (std::vector<int>::size_type i = 0; i != renderableObjects.size(); i++) {
		if (renderableObjects[i].first.castShadows) {
			VkBuffer currentVertexBuffer[] = { renderableObjects[i].second->getVertexBuffer() };
			VkDescriptorSet currentDescriptorSet = renderableObjects[i].second->getDescriptorSet();

			vkCmdBindVertexBuffers(offscreenPass.commandBuffer, 0, 1, currentVertexBuffer, offsets);
			vkCmdBindIndexBuffer(offscreenPass.commandBuffer, renderableObjects[i].second->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindDescriptorSets(offscreenPass.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenPipelineLayout, RENDERABLE_UBO, 1, &currentDescriptorSet, 0, nullptr);

			vkCmdDrawIndexed(offscreenPass.commandBuffer, renderableObjects[i].second->numIndices(), 1, 0, 0, 0);
		}
	} 

	vkCmdEndRenderPass(offscreenPass.commandBuffer);
	// Make sure color writes to the framebuffer are finished before using it as transfer source
	vulkanAPIHandler->transitionImageLayout(offscreenPass.commandBuffer, offscreenPass.color.image, OFFSCREEN_FB_COLOR_FORMAT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	// Copy region for the transfer from framebuffer to cube face
	VkImageCopy copyRegion = {};

	copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.srcSubresource.baseArrayLayer = 0;
	copyRegion.srcSubresource.mipLevel = 0;
	copyRegion.srcSubresource.layerCount = 1;
	copyRegion.srcOffset = { 0, 0, 0 };

	copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.dstSubresource.baseArrayLayer = faceIndex;
	copyRegion.dstSubresource.mipLevel = 0;
	copyRegion.dstSubresource.layerCount = 1;
	copyRegion.dstOffset = { 0, 0, 0 };

	copyRegion.extent.width = CUBE_MAP_TEX_DIM;
	copyRegion.extent.height = CUBE_MAP_TEX_DIM;
	copyRegion.extent.depth = 1;
	
	// Put image copy into command buffer
	vkCmdCopyImage(offscreenPass.commandBuffer,
				   offscreenPass.color.image,
				   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				   shadowCubeMapImages[lightIndex],
				   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				   1,
				   &copyRegion);

	// Transform framebuffer color attachment back 
	vulkanAPIHandler->transitionImageLayout(offscreenPass.commandBuffer, offscreenPass.color.image, OFFSCREEN_FB_COLOR_FORMAT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}

// Command buffer for rendering and copying all cube map faces
void Scene::buildOffscreenCommandBuffer() {
	if (offscreenPass.commandBuffer == VK_NULL_HANDLE) {
		offscreenPass.commandBuffer = vulkanAPIHandler->beginSingleTimeCommands(false);
	}
	
	if (offscreenPass.semaphore == VK_NULL_HANDLE) {
		// The semaphore is used to synchronize offscreen rendering. This happens before the color/main rendering
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		
		if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &offscreenPass.semaphore) != VK_SUCCESS) {
			throw std::runtime_error("failed to create offscreen semaphore");
		}
	}

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(offscreenPass.commandBuffer, &cmdBufInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin offscreen command buffer");
	}

	// Change image layout for all cubemap faces to transfer destination
	for (int i = 0; i < shadowCubeMapImages.size(); i++) {
		vulkanAPIHandler->transitionImageLayout(offscreenPass.commandBuffer, shadowCubeMapImages[i], OFFSCREEN_FB_COLOR_FORMAT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, NUM_CUBE_FACES);
	}

	for (uint32_t i = 0; i < shadowCubeMapImages.size(); i++) {
		for (uint32_t face = 0; face < NUM_CUBE_FACES; face++) {
			updateCubeFace(face, i);
		}
	}

	// Change image layout for all cubemap faces to shader read after they have been copied
	for (int i = 0; i < shadowCubeMapImages.size(); i++) {
		vulkanAPIHandler->transitionImageLayout(offscreenPass.commandBuffer, shadowCubeMapImages[i], OFFSCREEN_FB_COLOR_FORMAT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, NUM_CUBE_FACES);
	}

	if (vkEndCommandBuffer(offscreenPass.commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to end offscreen command buffer");
	}
}

void Scene::createOffscreenPipelineLayout() {
	// Offscreen pipeline layout. Uses push constants
	VkDescriptorSetLayout setLayouts[] = { getDescriptorSetLayout(DESC_LAYOUT_RENDERABLE), descriptorSetLayout };
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = std::size(setLayouts);
	pipelineLayoutInfo.pSetLayouts = setLayouts;

	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.size = sizeof(PushConstants);
	pushConstantRange.offset = 0;
	
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, offscreenPipelineLayout.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create offscreen pipeline layout!");
	}
}

void Scene::prepareOffscreenRenderpass() {
	// Color attachment
	VkAttachmentDescription osAttachments[2] = {};
	osAttachments[0].format = OFFSCREEN_FB_COLOR_FORMAT;
	osAttachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	osAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	osAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	osAttachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	osAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	osAttachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	osAttachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Depth attachment
	osAttachments[1].format = frameBufferDepthFormat;
	osAttachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	osAttachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	osAttachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	osAttachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	osAttachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	osAttachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	osAttachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = {};
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorReference;
	subpass.pDepthStencilAttachment = &depthReference;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 2;
	renderPassCreateInfo.pAttachments = osAttachments;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &offscreenPass.renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create offscreen render pass!");
	}
}

void Scene::prepareOffscreenPipeline(VkGraphicsPipelineCreateInfo pipelineInfo) {
	auto vertShaderCode = ShaderHandler::readFile("Shaders/Offscreen/vert.spv");
	auto fragShaderCode = ShaderHandler::readFile("Shaders/Offscreen/frag.spv");
	
	VDeleter<VkShaderModule> vertShaderModule{ device, vkDestroyShaderModule };
	VDeleter<VkShaderModule> fragShaderModule{ device, vkDestroyShaderModule };
	vulkanAPIHandler->createShaderModule(vertShaderCode, vertShaderModule);
	vulkanAPIHandler->createShaderModule(fragShaderCode, fragShaderModule);

	// Setting up shader modules
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	// Setting up offscreen viewports and scissors. These HAVE to be the dimensions of the texture, otherwise the shadow map breaks!
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = OFFSCREEN_FB_TEX_DIM;
	viewport.height = OFFSCREEN_FB_TEX_DIM;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent.width = OFFSCREEN_FB_TEX_DIM;
	scissor.extent.height = OFFSCREEN_FB_TEX_DIM;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.layout = offscreenPipelineLayout;
	pipelineInfo.renderPass = offscreenPass.renderPass;
	pipelineInfo.pViewportState = &viewportState;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, offscreenPipeline.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create offscreen pipeline!");
	}
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