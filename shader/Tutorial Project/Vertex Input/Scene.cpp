#include "Scene.h"
#include "VulkanAPIHandler.h"

Scene::Scene(VulkanAPIHandler* vulkanAPI) {
	vulkanAPIHandler = vulkanAPI;
	device = vulkanAPIHandler->getDevice();

	sceneUBO.lightPositions[0] = glm::vec4(400, 100, 400, 1.0);
	sceneUBO.lightColors[0] = glm::vec4(1, 1, 1, 1);

	frameBufferDepthFormat = vulkanAPIHandler->findDepthFormat();
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

	vkDestroyFramebuffer(device, offscreenPass.frameBuffer, nullptr);
	vkDestroyRenderPass(device, offscreenPass.renderPass, nullptr);
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
			if (CollisionHandler::checkCollision(std::dynamic_pointer_cast<Ghost>(ghost.second)->getCollisionRect(), 
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
	
	// TODO: Possibly just have the pointers class local first as they can then be placed in different vectors which is convenient for categorisation purposes.
	renderableObjects.emplace_back(
		std::make_pair(
			RENDERABLE_PACMAN, 
			std::make_shared<Pacman>(
				std::dynamic_pointer_cast<RenderableMaze>(renderableObjects[mazeIndex].second), 
				vulkanAPIHandler, 
				spawnPositions[0], 
				glm::vec3(30, 30, 30), 
				glm::vec4(1, 1, 0, 1))));
	
	
	renderableObjects.emplace_back(
		std::make_pair(
			RENDERABLE_GHOST, 
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
		std::make_pair(
			RENDERABLE_GHOST, 
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
		std::make_pair(
			RENDERABLE_GHOST, 
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

// Based on https://github.com/SaschaWillems/Vulkan/blob/master/shadowmappingomni/shadowmappingomni.cpp
// Contains a lot of code duplication from VulkanAPIHandler so refactoring the code here to use the VulkanAPIHandler would be nice, but not a priority yet
void Scene::prepareCubeMaps() {
	// 32 bit float format for higher precision
	VkFormat format = VK_FORMAT_R32_SFLOAT;

	// Cube map image description
	// TODO: Merge this with createImage()
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = format;
	imageCreateInfo.extent = { CUBE_MAP_TEX_DIM, CUBE_MAP_TEX_DIM, 1 };
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 6;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	if (vkCreateImage(device, &imageCreateInfo, nullptr, shadowCubeMapImage.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, shadowCubeMapImage, &memRequirements);

	VkMemoryAllocateInfo memAllocInfo = {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = memRequirements.size;
	memAllocInfo.memoryTypeIndex = vulkanAPIHandler->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(device, &memAllocInfo, nullptr, shadowCubeMapMemory.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(device, shadowCubeMapImage, shadowCubeMapMemory, 0);
	//TODO END

	// Setting up and recording the command buffer
	VkCommandBuffer layoutCmd = vulkanAPIHandler->beginSingleTimeCommands();

	// Create cube map image
	if (vkCreateImage(device, &imageCreateInfo, nullptr, shadowCubeMapImage.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create cubemap image");
	}

	vkGetImageMemoryRequirements(device, shadowCubeMapImage, &memRequirements);

	memAllocInfo.allocationSize = memRequirements.size;
	memAllocInfo.memoryTypeIndex = vulkanAPIHandler->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	if (vkAllocateMemory(device, &memAllocInfo, nullptr, shadowCubeMapMemory.replace()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate cube map memory");
	}
	vkBindImageMemory(device, shadowCubeMapImage, shadowCubeMapMemory, 0);

	// Image barrier for optimal image (target)	
	vulkanAPIHandler->transitionImageLayout(shadowCubeMapImage, frameBufferDepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6);

	// Flush command buffer
	vulkanAPIHandler->endSingleTimeCommands(layoutCmd);

	// Create sampler
	VkSamplerCreateInfo sampler = {};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.magFilter = CUBE_MAP_TEX_FILTER;
	sampler.minFilter = CUBE_MAP_TEX_FILTER;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	sampler.addressModeV = sampler.addressModeU;
	sampler.addressModeW = sampler.addressModeU;
	sampler.mipLodBias = 0.0f;
	sampler.maxAnisotropy = 0;
	sampler.compareOp = VK_COMPARE_OP_NEVER;
	sampler.minLod = 0.0f;
	sampler.maxLod = 1.0f;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	
	if (vkCreateSampler(device, &sampler, nullptr, shadowCubeMapSampler.replace()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create cubemap sampler");
	}

	// Create image view
	VkImageViewCreateInfo view = {};
	view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view.image = VK_NULL_HANDLE;
	view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	view.format = format;
	view.components = { VK_COMPONENT_SWIZZLE_R };
	view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	view.subresourceRange.layerCount = 6;
	view.image = shadowCubeMapImage;
	
	if (vkCreateImageView(device, &view, nullptr, shadowCubeMapImageView.replace()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create cubemap image view!");
	}
}

// Prepare a new framebuffer for offscreen rendering
// The contents of this framebuffer are then
// copied to the different cube map faces
void Scene::prepareOffscreenFramebuffer() {
	offscreenPass.width = OFFSCREEN_FB_TEX_DIM;
	offscreenPass.height = OFFSCREEN_FB_TEX_DIM;

	VkFormat frameBufferColorFormat = OFFSCREEN_FB_COLOR_FORMAT;

	// Color attachment
	// TODO: Merge with createImage
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = frameBufferColorFormat;
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
	colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	colorImageView.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	colorImageView.format = frameBufferColorFormat;
	colorImageView.flags = 0;
	colorImageView.subresourceRange = {};
	colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	colorImageView.subresourceRange.baseMipLevel = 0;
	colorImageView.subresourceRange.levelCount = 1;
	colorImageView.subresourceRange.baseArrayLayer = 0;
	colorImageView.subresourceRange.layerCount = 1;

	if (vkCreateImage(device, &imageCreateInfo, nullptr, &offscreenPass.color.image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create offscreen color image");
	}

	VkMemoryAllocateInfo memAlloc = {};
	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkMemoryRequirements memReqs;

	vkGetImageMemoryRequirements(device, offscreenPass.color.image, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = vulkanAPIHandler->findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	if(vkAllocateMemory(device, &memAlloc, nullptr, &offscreenPass.color.memory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate offscreen color memory");
	}
	if (vkBindImageMemory(device, offscreenPass.color.image, offscreenPass.color.memory, 0) != VK_SUCCESS) {
		throw std::runtime_error("failed to bind offscreen color memory");
	}
	// TODO END

	VkCommandBuffer layoutCmd = vulkanAPIHandler->beginSingleTimeCommands();

	vulkanAPIHandler->transitionImageLayout(offscreenPass.color.image, frameBufferColorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	colorImageView.image = offscreenPass.color.image;

	if (vkCreateImageView(device, &colorImageView, nullptr, &offscreenPass.color.view) != VK_SUCCESS) {
		throw std::runtime_error("failed to create offscreen color image view");
	}

	// Depth stencil attachment
	imageCreateInfo.format = frameBufferDepthFormat;
	imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	VkImageViewCreateInfo depthStencilView = {};

	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthStencilView.format = frameBufferDepthFormat;
	depthStencilView.flags = 0;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = 1;

	if (vkCreateImage(device, &imageCreateInfo, nullptr, &offscreenPass.depth.image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create offscreen depth image");
	}

	vkGetImageMemoryRequirements(device, offscreenPass.depth.image, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = vulkanAPIHandler->findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	if (vkAllocateMemory(device, &memAlloc, nullptr, &offscreenPass.depth.memory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate offscreen depth memory");
	}
	if (vkBindImageMemory(device, offscreenPass.depth.image, offscreenPass.depth.memory, 0) != VK_SUCCESS) {
		throw std::runtime_error("failed to bind offscreen depth memory");
	}

	vulkanAPIHandler->transitionImageLayout(offscreenPass.depth.image, frameBufferDepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	// Stop commandBuffer recording
	vulkanAPIHandler->endSingleTimeCommands(layoutCmd);

	depthStencilView.image = offscreenPass.depth.image;
	if (vkCreateImageView(device, &depthStencilView, nullptr, &offscreenPass.depth.view) != VK_SUCCESS) {
		throw std::runtime_error("failed to create offscreen depth image view");
	}

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
void Scene::updateCubeFace(uint32_t faceIndex) {
	VkClearValue clearValues[2];
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkFormat frameBufferColorFormat = OFFSCREEN_FB_COLOR_FORMAT;

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
	switch (faceIndex)
	{
	case 0: // POSITIVE_X
		viewMatrix = glm::rotate(viewMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	case 1:	// NEGATIVE_X
		viewMatrix = glm::rotate(viewMatrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	case 2:	// POSITIVE_Y
		viewMatrix = glm::rotate(viewMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	case 3:	// NEGATIVE_Y
		viewMatrix = glm::rotate(viewMatrix, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	case 4:	// POSITIVE_Z
		viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	case 5:	// NEGATIVE_Z
		viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		break;
	}

	// Render scene from cube face's point of view
	vkCmdBeginRenderPass(offscreenPass.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Update shader push constant block
	// Contains current face view matrix
	vkCmdPushConstants(
		offscreenPass.commandBuffer,
		offscreenPipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(glm::mat4),
		&viewMatrix);

	vkCmdBindPipeline(offscreenPass.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenPipeline);
	vkCmdBindDescriptorSets(offscreenPass.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenPipelineLayout, 0, 1, &offscreenDescriptorSet, 0, NULL);

	VkDeviceSize offsets[] = { 0 };
	for (auto& renderable : renderableObjects) {
		VkBuffer currentVertexBuffer[] = { renderable.second->getVertexBuffer() };
		VkDescriptorSet currentDescriptorSet = renderable.second->getDescriptorSet();

		vkCmdBindVertexBuffers(offscreenPass.commandBuffer, 0, 1, currentVertexBuffer, offsets);
		vkCmdBindIndexBuffer(offscreenPass.commandBuffer, renderable.second->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(offscreenPass.commandBuffer, renderable.second->numIndices(), 1, 0, 0, 0);
	}

	vkCmdEndRenderPass(offscreenPass.commandBuffer);
	// Make sure color writes to the framebuffer are finished before using it as transfer source
	vulkanAPIHandler->transitionImageLayout(offscreenPass.color.image, frameBufferColorFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	// Copy region for transfer from framebuffer to cube face
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
	vkCmdCopyImage(
		offscreenPass.commandBuffer,
		offscreenPass.color.image,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		shadowCubeMapImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&copyRegion);

	// Transform framebuffer color attachment back 
	vulkanAPIHandler->transitionImageLayout(offscreenPass.color.image, frameBufferColorFormat, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
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