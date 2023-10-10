#include "Context.h"

std::fstream Context::output;

Context::Context() {
	output.open("E:/log.txt", std::ios::out);

	glfwInit();

#ifdef _DEBUG
	enableValidationLayers = true;
#endif

	validationLayers.push_back("VK_LAYER_KHRONOS_validation");

	uint32_t glfwExtensionCount = 0;
	const char **glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	instanceExtensions = extensions;

	deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

Context::~Context() { output.close(); }

void Context::create(){
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createCommandPool();
}

void Context::destroy(){
	vkDestroyCommandPool(device, commandPool, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);

	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, nullptr);
	}

	vkDestroyInstance(instance, nullptr);
}

VkInstance Context::getInstance()const { return instance; }
VkPhysicalDevice Context::getPhysicalDevice()const { return gpu; }
VkDevice Context::getDevice()const { return device; }
VkSurfaceKHR Context::getSurface()const { return surface; }
GLFWwindow* Context::getWindow()const { return window; }
VkCommandPool Context::getCommandPool()const { return commandPool; }

uint32_t Context::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
			properties) == properties) {
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type!");
}

void Context::createInstance() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	bool flag = true;
	for (const char *layerName : validationLayers) {
		bool layerFound = false;
		for (const auto &layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}
		if (!layerFound) {
			flag = false;
			break;
		}
	}
	if (enableValidationLayers && !flag) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = debugCallback;
		createInfo.pNext = &debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}
}

void Context::setupDebugMessenger() {
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	VkResult result;
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		result = func(instance, &createInfo, nullptr, &debugMessenger);
	}
	else {
		result = VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

void Context::createSurface() {
	VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

void Context::pickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto &device : devices) {
		// check extensions' suppport
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		for (const auto &extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}
		if (!requiredExtensions.empty()) continue;

		// check anisotropy
		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
		if (supportedFeatures.samplerAnisotropy == VK_FALSE) continue;

		// check the support for swap chain
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		if (formatCount == 0 || presentModeCount == 0) continue;

		// check queue family
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
		uint32_t graphicsFamilyTemp = UINT32_MAX;
		uint32_t presentFamilyTemp = UINT32_MAX;
		int i = 0;
		for (const auto &queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				graphicsFamilyTemp = i;
			}
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (presentSupport) {
				presentFamilyTemp = i;
			}

			if (graphicsFamilyTemp != UINT32_MAX && presentFamilyTemp != UINT32_MAX) {
				break;
			}
			i++;
		}
		if (graphicsFamilyTemp == UINT32_MAX || presentFamilyTemp == UINT32_MAX) continue;

		gpu = device;
		graphicsFamily = graphicsFamilyTemp;
		presentFamily = presentFamilyTemp;
		break;
	}

	if (gpu == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
	vkGetPhysicalDeviceProperties(gpu, &gpuProperties);
	vkGetPhysicalDeviceMemoryProperties(gpu, &memProperties);
	VkSampleCountFlags counts =
		gpuProperties.limits.framebufferColorSampleCounts &
		gpuProperties.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) { msaaSamples = VK_SAMPLE_COUNT_64_BIT; }
	else if (counts & VK_SAMPLE_COUNT_32_BIT) { msaaSamples = VK_SAMPLE_COUNT_32_BIT; }
	else if (counts & VK_SAMPLE_COUNT_16_BIT) { msaaSamples = VK_SAMPLE_COUNT_16_BIT; }
	else if (counts & VK_SAMPLE_COUNT_8_BIT) { msaaSamples = VK_SAMPLE_COUNT_8_BIT;	}
	else if (counts & VK_SAMPLE_COUNT_4_BIT) { msaaSamples = VK_SAMPLE_COUNT_4_BIT; }
	else if (counts & VK_SAMPLE_COUNT_2_BIT) { msaaSamples = VK_SAMPLE_COUNT_2_BIT; }
	else { msaaSamples = VK_SAMPLE_COUNT_1_BIT; }
}

void Context::createLogicalDevice() {
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { graphicsFamily,presentFamily };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount =	static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(gpu, &createInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(device, presentFamily, 0, &presentQueue);
}

void Context::createCommandPool() {
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = graphicsFamily;

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}

void Context::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<Context*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}

VkBool32 Context::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	output << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

