#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#include "stb_image.h"
#include "tiny_obj_loader.h"
#include "Renderable.h"
#include "VulkanAPIHandler.h"

Renderable::Renderable() {
}

Renderable::Renderable(VulkanAPIHandler* vkAPIHandler, glm::vec4 pos, std::string texPath, std::string meshPath, bool invertedNormals) {
	vulkanAPIHandler = vkAPIHandler;
	device = vkAPIHandler->getDevice();
	texturePath = texPath;
	modelPath = meshPath;
	position = pos;

	loadModel(invertedNormals);
}

Renderable::Renderable(VulkanAPIHandler * vkAPIHandler, glm::vec4 pos, std::string texPath, std::string meshPath, glm::vec3 renderableScale, glm::vec4 c, bool invertedNormals) {
	vulkanAPIHandler = vkAPIHandler;
	device = vkAPIHandler->getDevice();
	texturePath = texPath;
	modelPath = meshPath;
	position = pos;
	baseColor = c;
	scale = renderableScale;

	loadModel(invertedNormals);
}

Renderable::Renderable(VulkanAPIHandler* vkAPIHandler, glm::vec4 pos, std::string texPath) {
	vulkanAPIHandler = vkAPIHandler;
	device = vkAPIHandler->getDevice();
	texturePath = texPath;
	position = pos;
}

Renderable::~Renderable() {	
}

void Renderable::createVertexIndexBuffers() {
	// Create Vertex buffer
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VDeleter<VkBuffer> vertexStagingBuffer{ device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> vertexStagingBufferMemory{ device, vkFreeMemory };
	
	vulkanAPIHandler->createBuffer(
		bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		vertexStagingBuffer, 
		vertexStagingBufferMemory);

	void* vertexData;
	vkMapMemory(device, vertexStagingBufferMemory, 0, bufferSize, 0, &vertexData);
	memcpy(vertexData, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, vertexStagingBufferMemory);

	vulkanAPIHandler->createBuffer(
		bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		vertexBuffer, 
		vertexBufferMemory);

	vulkanAPIHandler->copyBuffer(vertexStagingBuffer, vertexBuffer, bufferSize);

	// Create Index buffer
	bufferSize = sizeof(indices[0]) * indices.size();

	VDeleter<VkBuffer> indexStagingBuffer{ device, vkDestroyBuffer };
	VDeleter<VkDeviceMemory> indexStagingBufferMemory{ device, vkFreeMemory };
	vulkanAPIHandler->createBuffer(
		bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		indexStagingBuffer, 
		indexStagingBufferMemory);

	void* indexData;
	vkMapMemory(device, indexStagingBufferMemory, 0, bufferSize, 0, &indexData);
	memcpy(indexData, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, indexStagingBufferMemory);

	vulkanAPIHandler->createBuffer(
		bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		indexBuffer, 
		indexBufferMemory);

	vulkanAPIHandler->copyBuffer(indexStagingBuffer, indexBuffer, bufferSize);
}

void Renderable::createUniformBuffer() {
	VkDeviceSize bufferSize = sizeof(RenderableUBO);

	vulkanAPIHandler->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformStagingBuffer, uniformStagingBufferMemory);
	vulkanAPIHandler->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, uniformBuffer, uniformBufferMemory);
}

VkBuffer Renderable::getVertexBuffer() {
	return vertexBuffer;
}

VkBuffer Renderable::getIndexBuffer() {
	return indexBuffer;
}

int Renderable::numIndices() {
	return indices.size();
}

void Renderable::loadModel(bool invertNormals) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelPath.c_str())) {
		throw std::runtime_error(err);
	}

	std::unordered_map<Vertex, int> uniqueVertices = {};

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex = {};

			// Vertices consist of 3 floats so we need to offset by multiplying the index with 3
			vertex.position = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2],
				1.0f
			};

			vertex.color = baseColor;

			if (attrib.texcoords.size() != 0) {
				// The same goes for texture coordinates where we use 2 instead
				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					// The origin of texture coordinates in vulkan is in the top left corner so we are flipping this vertical component
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
					0,
					1
				};
			}
			
			if (attrib.normals.size() != 0) {
				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2],
					0
				};
				vertex.normal *= (invertNormals ? -1.f : 1.f);
			}

			// Performing vertex deduplication
			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = vertices.size();
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}
}

void Renderable::createTextureImage() {
	const int BYTES_PER_PIXEL = 4;
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * BYTES_PER_PIXEL;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	VDeleter<VkImage> stagingImage{ device, vkDestroyImage };
	VDeleter<VkDeviceMemory> stagingImageMemory{ device, vkFreeMemory };
	vulkanAPIHandler->createImage(
		texWidth,
		texHeight,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_LINEAR,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingImage,
		stagingImageMemory);

	VkImageSubresource subresource = {};
	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource.mipLevel = 0;
	subresource.arrayLayer = 0;

	VkSubresourceLayout stagingImageLayout;
	vkGetImageSubresourceLayout(device, stagingImage, &subresource, &stagingImageLayout);

	void* data;
	vkMapMemory(device, stagingImageMemory, 0, imageSize, 0, &data);

	// Handling cases where the image has padding bytes
	if (stagingImageLayout.rowPitch == texWidth * BYTES_PER_PIXEL) {
		memcpy(data, pixels, (size_t)imageSize);
	}
	else {
		uint8_t* dataBytes = reinterpret_cast<uint8_t*>(data);

		for (int y = 0; y < texHeight; y++) {
			memcpy(&dataBytes[y * stagingImageLayout.rowPitch], &pixels[y * texWidth * BYTES_PER_PIXEL], texWidth * BYTES_PER_PIXEL);
		}
	}
	vkUnmapMemory(device, stagingImageMemory);
	stbi_image_free(pixels);

	// Creating the final texture image
	vulkanAPIHandler->createImage(
		texWidth,
		texHeight,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		textureImage,
		textureImageMemory
	);

	// Copying staging image to the texture image
	vulkanAPIHandler->transitionImageLayout(stagingImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vulkanAPIHandler->transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	vulkanAPIHandler->copyImage(stagingImage, textureImage, texWidth, texHeight);
	vulkanAPIHandler->transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Renderable::createTextureImageView() {
	vulkanAPIHandler->createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, textureImageView);
}

void Renderable::createTextureSampler() {
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, textureSampler.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

void Renderable::createDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = bindings.size();
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, descriptorSetLayout.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void Renderable::createDescriptorSet(VkDescriptorPool descriptorPool) {
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor set!");
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(RenderableUBO);

	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = textureImageView;
	imageInfo.sampler = textureSampler;

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
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void Renderable::update(float deltaTime) {
}

VkDescriptorSet Renderable::getDescriptorSet() {
	return descriptorSet;
}

VkDescriptorSetLayout Renderable::getDescriptorLayout() {
	return descriptorSetLayout;
}

void Renderable::updateUniformBuffer(glm::mat4 projectionMatrix, glm::mat4 viewMatrix) {
	RenderableUBO ubo = {};
	
	/*
	modelMatrix = 
		glm::translate(glm::mat4(1.0f), position) * 
		glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0, 1, 0)) * 
		glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0)) * 
		glm::scale(glm::mat4(1.0f), scale);
	*/

	modelMatrix = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), scale);
	ubo.mvp = projectionMatrix * viewMatrix * modelMatrix;
	ubo.viewMatrix = viewMatrix;
	ubo.modelMatrix = modelMatrix;

	void* data;
	vkMapMemory(device, uniformStagingBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device, uniformStagingBufferMemory);

	vulkanAPIHandler->copyBuffer(uniformStagingBuffer, uniformBuffer, sizeof(ubo));
}
