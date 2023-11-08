#pragma once

#include <vector>
#include <string>
#include <set>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include "base/glfw_context.h"

class Context
{
public:
	Context();
	~Context();
	void create(GlfwContext* gc);
	void destroy();
	VkInstance getInstance()const;
	VkPhysicalDevice getPhysicalDevice()const;
	VkDevice getDevice()const;
	VkSurfaceKHR getSurface()const;
	VkCommandPool getCommandPool()const;

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

private:
	void createInstance();
	void setupDebugMessenger();
	void createSurface(GLFWwindow* window);
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createCommandPool();

	static VKAPI_ATTR VkBool32 VKAPI_CALL
		debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);
public:

	VkDebugUtilsMessengerEXT debugMessenger;

	// instance
	VkInstance instance{ VK_NULL_HANDLE };
	bool enableValidationLayers{ false };
	std::vector<const char*> validationLayers;
	std::vector<const char*> instanceExtensions;

	// surface
	VkSurfaceKHR surface{ VK_NULL_HANDLE };

	/// @name physical device
	///@{
	VkPhysicalDevice gpu{ VK_NULL_HANDLE };
	VkPhysicalDeviceProperties gpuProperties;
	VkPhysicalDeviceMemoryProperties memProperties;
	std::vector<const char*> deviceExtensions;
	uint32_t graphicsFamily = UINT32_MAX;
	uint32_t presentFamily = UINT32_MAX;
	VkSampleCountFlagBits msaaSamples{ VK_SAMPLE_COUNT_1_BIT };
	///@}

	// logical device
	VkDevice device{ VK_NULL_HANDLE };
	VkQueue graphicsQueue{ VK_NULL_HANDLE };
	VkQueue presentQueue{ VK_NULL_HANDLE };
	VkCommandPool commandPool{ VK_NULL_HANDLE };

	VmaAllocator allocator;

	static std::fstream output;
};

