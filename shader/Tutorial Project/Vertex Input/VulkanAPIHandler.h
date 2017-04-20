#pragma once
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define RENDERABLE_UBO		0
#define SCENE_UBO			1

#define CUBE_MAP_TEX_DIM    1024
#define CUBE_MAP_TEX_FILTER VK_FILTER_LINEAR

#define OFFSCREEN_FB_TEX_DIM      CUBE_MAP_TEX_DIM
#define OFFSCREEN_FB_COLOR_FORMAT VK_FORMAT_R32_SFLOAT

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <map>
#include <set>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <unordered_map>
#include <memory>
#include "VDeleter.h"
#include "consts.h"
#include "ShaderHandler.h"
#include "Structs.h"
#include "Scene.h"

/*
const std::vector<Vertex> vertices = {
	{ { -0.5f, -0.5f, 0.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f } },
	{ { 0.5f, -0.5f, 0.0f },{ 0.0f, 1.0f, 0.0f },{ 0.0f, 1.0f } },
	{ { 0.5f, 0.5f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f } },
	{ { -0.5f, 0.5f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 1.0f, 0.0f } },

	{ { -0.5f, -0.5f, -0.5f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f } },
	{ { 0.5f, -0.5f, -0.5f },{ 0.0f, 1.0f, 0.0f },{ 0.0f, 1.0f } },
	{ { 0.5f, 0.5f, -0.5f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f } },
	{ { -0.5f, 0.5f, -0.5f },{ 1.0f, 1.0f, 1.0f },{ 1.0f, 0.0f } }
};*/

/*
// https://github.com/Overv/VulkanTutorial/blob/master/images/vertex_vs_index.svg
const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0,
	4, 5, 6, 6, 7, 4
}; */

// Class primarily consisting of snippets from https://vulkan-tutorial.com
class VulkanAPIHandler {
public:
	VulkanAPIHandler(GLFWwindow* GLFWwindow);
	~VulkanAPIHandler();
	void drawFrame(); 
	void updateUniformBuffers();
	void update(float deltaTime);
	static void onWindowResized(GLFWwindow* window, int width, int height);
	VulkanAPIHandler* getPtr();
	VkDevice getDevice();
	VkCommandPool getCommandPool();
	void handleInput(GLFWKeyEvent event);

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, int subResourceLayerCount = 1, bool hasDepthStencilBit = false);
	void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, int subResourceLayerCount = 1, bool hasDepthStencilBit = false);
	void createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VDeleter<VkImageView>& imageView);

	void createBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VDeleter<VkBuffer>& buffer,
		VDeleter<VkDeviceMemory>& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void createImage(
		uint32_t width,
		uint32_t height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VDeleter<VkImage>& image,
		VDeleter<VkDeviceMemory>& imageMemory);
	void copyImage(VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height);
	void copyImage(VkImage srcImage, VkImage dstImage, VkImageCopy copyRegion);

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkFormat findDepthFormat();

	VkCommandBuffer beginSingleTimeCommands(bool startRecording = true);
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	void createShaderModule(const std::vector<char>& code, VDeleter<VkShaderModule>& shaderModule);
private:
	//********************
	// Consts
	//********************
	#ifdef NDEBUG
	const bool enableValidationLayers = false;
	#else
	const bool enableValidationLayers = true;
	#endif

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_LUNARG_standard_validation"
	};

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	//********************
	// Structs
	//********************
	// Used for checking queue families
	struct QueueFamilyIndices {
		int graphicsFamily = -1;
		int presentFamily = -1;

		bool isComplete() {
			return graphicsFamily >= 0 && presentFamily >= 0;
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	//********************
	// Member variables
	//********************

	Scene* scene;
	GLFWwindow* window;

	VDeleter<VkInstance> instance{ vkDestroyInstance };
	VDeleter<VkDebugReportCallbackEXT> callback{ instance, DestroyDebugReportCallbackEXT };
	
	// The surface is an abstract type that we present rendered images to
	VDeleter<VkSurfaceKHR> surface{ instance, vkDestroySurfaceKHR };
	
	VkPhysicalDevice physicalDevice;
	VDeleter<VkDevice> device{ vkDestroyDevice };

	VkQueue graphicsQueue;
	VkQueue presentationQueue;

	VDeleter<VkSwapchainKHR> swapChain{ device, vkDestroySwapchainKHR };
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VDeleter<VkImageView>> swapChainImageViews;
	std::vector<VDeleter<VkFramebuffer>> swapChainFramebuffers;

	VDeleter<VkRenderPass> renderPass{ device, vkDestroyRenderPass };
	VDeleter<VkPipelineLayout> pipelineLayout{ device, vkDestroyPipelineLayout };
	VDeleter<VkPipeline> graphicsPipeline{ device, vkDestroyPipeline };

	VDeleter<VkCommandPool> commandPool{ device, vkDestroyCommandPool };
	std::vector<VkCommandBuffer> commandBuffers;

	VDeleter<VkDescriptorPool> descriptorPool{ device, vkDestroyDescriptorPool };

	VDeleter<VkSemaphore> imageAvailableSemaphore{ device, vkDestroySemaphore };
	VDeleter<VkSemaphore> renderFinishedSemaphore{ device, vkDestroySemaphore };

	VDeleter<VkImage> depthImage{ device, vkDestroyImage };
	VDeleter<VkDeviceMemory> depthImageMemory{ device, vkFreeMemory };
	VDeleter<VkImageView> depthImageView{ device, vkDestroyImageView };


	//********************
	// Private methods
	//********************
	void initVulkan();
	void createInstance();
	bool checkValidationLayerSupport();
	void setupDebugCallback();
	void pickPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	void createLogicalDevice();
	void createSurface();
	void createSwapChain();
	void createImageViews();
	void createRenderPass();
	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createFramebuffers();
	void createCommandPool();
	void createDepthResources();
	void createTextureImages();
	void createTextureImageViews();
	void createTextureSamplers();
	void createCommandBuffers();
	void createVertexIndexBuffers();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSet();
	void createSemaphores();
	void recreateSwapChain();
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	bool hasStencilComponent(VkFormat format);

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	std::vector<const char*> getRequiredExtensions();

	void transitionImageLayoutCore(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer commandBuffer, int subresourceLayerCount = 1, bool hasDepthStencilBit = false);

	// For choosing color depth
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	// e.g: This is where we can choose double buffering (vsync) or triple buffering
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);

	// The swap extent is the resolution of the swap chain images and 
	// it's almost always exactly equal to the resolution of the window that we're drawing to.
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	// Debug related functions and callbacks.
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData);
	
	VkResult CreateDebugReportCallbackEXT(
		VkInstance instance,
		const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugReportCallbackEXT* pCallback);

	static void DestroyDebugReportCallbackEXT(
		VkInstance instance,
		VkDebugReportCallbackEXT callback,
		const VkAllocationCallbacks* pAllocator);
};