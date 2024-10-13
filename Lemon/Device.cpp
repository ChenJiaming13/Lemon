#include "pch.h"
#include "Device.h"
#include "GlfwWindow.h"

VkResult createDebugUtilsMessengerEXT(
	VkInstance vInstance,
	const VkDebugUtilsMessengerCreateInfoEXT* vCreateInfo,
	const VkAllocationCallbacks* vAllocator,
	VkDebugUtilsMessengerEXT* vDebugMessenger
)
{
	if (const auto Func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(vInstance, "vkCreateDebugUtilsMessengerEXT")); Func != nullptr)
	{
		return Func(vInstance, vCreateInfo, vAllocator, vDebugMessenger);
	}
	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void destroyDebugUtilsMessengerEXT(
	VkInstance vInstance,
	VkDebugUtilsMessengerEXT vDebugMessenger,
	const VkAllocationCallbacks* vAllocator
)
{
	if (const auto Func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(vInstance, "vkDestroyDebugUtilsMessengerEXT")); Func != nullptr)
	{
		Func(vInstance, vDebugMessenger, vAllocator);
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT vMessageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT vMessageType,
	const VkDebugUtilsMessengerCallbackDataEXT* vCallbackData,
	void* vUserData)
{
	if (vMessageSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		spdlog::info("[VALIDATION] {}", vCallbackData->pMessage);
		return VK_TRUE;
	}
	if (vMessageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		spdlog::warn("[VALIDATION] {}", vCallbackData->pMessage);
	else if (vMessageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		spdlog::error("[VALIDATION] {}", vCallbackData->pMessage);
	return VK_FALSE;
}

bool Lemon::CDevice::init(const CGlfwWindow* vWindow)
{
	std::vector<const char*> InstanceExtensions;
	__getRequiredInstanceExtensions(InstanceExtensions);
	if (!__createInstance(InstanceExtensions)) return false;
	if (!__setupDebugMessenger()) return false;

	vWindow->createSurface(m_Instance, &m_Surface);

	std::vector<const char*> DeviceExtensions;
	__getRequiredDeviceExtensions(DeviceExtensions);
	if (!__pickPhysicalDevice(DeviceExtensions)) return false;

	__findQueueFamilyIndices(m_QueueFamilyIndices, m_PhysicalDevice, m_Surface);

	if (!__createLogicalDevice(DeviceExtensions)) return false;
	if (!__getQueue()) return false;
	if (!__createCommandPool()) return false;
	return true;
}

void Lemon::CDevice::cleanup()
{
	vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
	m_CommandPool = VK_NULL_HANDLE;
	m_GraphicsQueue = VK_NULL_HANDLE;
	m_PresentQueue = VK_NULL_HANDLE;
	vkDestroyDevice(m_Device, nullptr);
	m_Device = VK_NULL_HANDLE;
	m_PhysicalDevice = VK_NULL_HANDLE;
	if (m_EnableValidationLayers)
	{
		destroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
		m_DebugMessenger = VK_NULL_HANDLE;
	}
	vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
	m_Surface = VK_NULL_HANDLE;
	vkDestroyInstance(m_Instance, nullptr);
	m_Instance = VK_NULL_HANDLE;
	m_QueueFamilyIndices = {};
}

bool Lemon::CDevice::allocatePrimaryCommandBuffer(uint32_t vCommandBufferCount, VkCommandBuffer* voCommandBuffers) const
{
	VkCommandBufferAllocateInfo AllocateInfo{};
	AllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	AllocateInfo.commandPool = m_CommandPool;
	AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AllocateInfo.commandBufferCount = vCommandBufferCount;

	if (vkAllocateCommandBuffers(m_Device, &AllocateInfo, voCommandBuffers) != VK_SUCCESS)
	{
		spdlog::error("failed to allocate command buffers!");
		return false;
	}
	return true;
}

VkCommandBuffer Lemon::CDevice::beginSingleTimeCommandBuffer() const
{
	VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
	if (!allocatePrimaryCommandBuffer(1, &CommandBuffer)) return VK_NULL_HANDLE;

	VkCommandBufferBeginInfo BeginInfo{};
	BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(CommandBuffer, &BeginInfo);
	return CommandBuffer;
}

void Lemon::CDevice::endAndSubmitCommandBuffer(VkCommandBuffer vCommandBuffer) const
{
	vkEndCommandBuffer(vCommandBuffer);

	VkSubmitInfo SubmitInfo{};
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &vCommandBuffer;

	vkQueueSubmit(m_GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_GraphicsQueue);

	vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &vCommandBuffer);
}

void Lemon::CDevice::querySwapChainSupportDetails(SSwapChainSupportDetails& voDetails) const
{
	__querySwapChainSupportDetails(voDetails, m_PhysicalDevice, m_Surface);
}

bool Lemon::CDevice::__createInstance(const std::vector<const char*>& vInstanceExtensions)
{
	if (m_EnableValidationLayers && !__checkValidationLayerSupport())
	{
		throw std::runtime_error("validation layers requested, but not available!");
	}

	VkApplicationInfo AppInfo{};
	AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	AppInfo.pApplicationName = "Lemon";
	AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	AppInfo.pEngineName = "No Engine";
	AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	AppInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	CreateInfo.pApplicationInfo = &AppInfo;
	CreateInfo.enabledExtensionCount = static_cast<uint32_t>(vInstanceExtensions.size());
	CreateInfo.ppEnabledExtensionNames = vInstanceExtensions.data();

	std::vector<const char*> ValidationLayers;
	if (m_EnableValidationLayers)
	{
		__getValidationLayers(ValidationLayers);
		CreateInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
		CreateInfo.ppEnabledLayerNames = ValidationLayers.data();
		VkDebugUtilsMessengerCreateInfoEXT DebugCreateInfo;
		__populateDebugMessengerCreateInfo(DebugCreateInfo);
		CreateInfo.pNext = &DebugCreateInfo;
	}
	else
	{
		CreateInfo.enabledLayerCount = 0;
		CreateInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&CreateInfo, nullptr, &m_Instance) != VK_SUCCESS)
	{
		spdlog::error("failed to create instance!");
		return false;
	}
	spdlog::info("created instance");
	return true;
}

bool Lemon::CDevice::__setupDebugMessenger()
{
	if (!m_EnableValidationLayers) return true;
	VkDebugUtilsMessengerCreateInfoEXT DebugCreateInfo;
	__populateDebugMessengerCreateInfo(DebugCreateInfo);
	if (createDebugUtilsMessengerEXT(m_Instance, &DebugCreateInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS)
	{
		spdlog::error("failed to set up debug messenger!");
		return false;
	}
	spdlog::info("created debug messenger");
	return true;
}

bool Lemon::CDevice::__pickPhysicalDevice(const std::vector<const char*>& vDeviceExtensions)
{
	uint32_t PhysicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(m_Instance, &PhysicalDeviceCount, nullptr);
	if (PhysicalDeviceCount == 0)
	{
		spdlog::error("failed to find GPUs with Vulkan support!");
		return false;
	}
	std::vector<VkPhysicalDevice> PhysicalDevices(PhysicalDeviceCount);
	vkEnumeratePhysicalDevices(m_Instance, &PhysicalDeviceCount, PhysicalDevices.data());
	for (const auto& PhysicalDevice : PhysicalDevices)
	{
		if (__isDeviceSuitable(PhysicalDevice, m_Surface, vDeviceExtensions))
		{
			m_PhysicalDevice = PhysicalDevice;
			break;
		}
	}
	if (m_PhysicalDevice == VK_NULL_HANDLE)
	{
		spdlog::error("failed to find a suitable GPU!");
		return false;
	}
	{
		VkPhysicalDeviceProperties Properties;
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &Properties);
		spdlog::info("picked physical device: {}", Properties.deviceName);
	}
	return true;
}

bool Lemon::CDevice::__createLogicalDevice(const std::vector<const char*>& vDeviceExtensions)
{
	_ASSERTE(m_PhysicalDevice && m_Surface);
	if (!m_QueueFamilyIndices._GraphicsFamily.has_value() || !m_QueueFamilyIndices._PresentFamily.has_value())
		return false;
	const std::set UniqueQueueFamilyIndices = { m_QueueFamilyIndices._GraphicsFamily.value(), m_QueueFamilyIndices._PresentFamily.value() };
	std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
	constexpr float QueuePriority = 1.0f;
	for (const uint32_t QueueFamilyIndex : UniqueQueueFamilyIndices)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = QueueFamilyIndex;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &QueuePriority;
		QueueCreateInfos.push_back(queueCreateInfo);
	}

	constexpr VkPhysicalDeviceFeatures DeviceFeatures{};

	VkDeviceCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	CreateInfo.queueCreateInfoCount = static_cast<uint32_t>(QueueCreateInfos.size());
	CreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
	CreateInfo.pEnabledFeatures = &DeviceFeatures;
	CreateInfo.enabledExtensionCount = static_cast<uint32_t>(vDeviceExtensions.size());
	CreateInfo.ppEnabledExtensionNames = vDeviceExtensions.data();
	CreateInfo.enabledLayerCount = 0;

	if (vkCreateDevice(m_PhysicalDevice, &CreateInfo, nullptr, &m_Device) != VK_SUCCESS)
	{
		spdlog::error("failed to create logical device!");
		return false;
	}
	spdlog::info("created logical device");
	return true;
}

bool Lemon::CDevice::__getQueue()
{
	_ASSERTE(m_Device);
	if (!m_QueueFamilyIndices._GraphicsFamily.has_value() || !m_QueueFamilyIndices._PresentFamily.has_value())
		return false;
	vkGetDeviceQueue(m_Device, m_QueueFamilyIndices._GraphicsFamily.value(), 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_Device, m_QueueFamilyIndices._PresentFamily.value(), 0, &m_PresentQueue);
	return m_GraphicsQueue && m_PresentQueue;
}

bool Lemon::CDevice::__createCommandPool()
{
	if (!m_QueueFamilyIndices._GraphicsFamily.has_value()) return false;

	VkCommandPoolCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	CreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	CreateInfo.queueFamilyIndex = m_QueueFamilyIndices._GraphicsFamily.value();

	if (vkCreateCommandPool(m_Device, &CreateInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
	{
		spdlog::error("failed to create graphics command pool!");
		return false;
	}
	spdlog::info("created command pool");
	return true;
}

bool Lemon::CDevice::__checkValidationLayerSupport()
{
	uint32_t LayerCount;
	vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);
	std::vector<VkLayerProperties> AvailableLayers(LayerCount);
	vkEnumerateInstanceLayerProperties(&LayerCount, AvailableLayers.data());
	std::vector<const char*> ValidationLayers;
	__getValidationLayers(ValidationLayers);
	for (const char* LayerName : ValidationLayers)
	{
		bool LayerFound = false;
		for (const auto& LayerProperties : AvailableLayers)
		{
			if (strcmp(LayerName, LayerProperties.layerName) == 0)
			{
				LayerFound = true;
				break;
			}
		}
		if (!LayerFound)
		{
			return false;
		}
	}
	return true;
}

void Lemon::CDevice::__getValidationLayers(std::vector<const char*>& voValidationLayers)
{
	voValidationLayers = { "VK_LAYER_KHRONOS_validation" };
}

void Lemon::CDevice::__getRequiredInstanceExtensions(std::vector<const char*>& voInstanceExtensions)
{
	voInstanceExtensions.clear();
	CGlfwWindow::getRequiredInstanceExtensions(voInstanceExtensions);
	if (m_EnableValidationLayers)
	{
		voInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
}

void Lemon::CDevice::__getRequiredDeviceExtensions(std::vector<const char*>& voDeviceExtensions)
{
	voDeviceExtensions = std::vector{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
}

void Lemon::CDevice::__populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& vDebugCreateInfo)
{
	vDebugCreateInfo = {};
	vDebugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	vDebugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	vDebugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	vDebugCreateInfo.pfnUserCallback = DebugCallback;
}

bool Lemon::CDevice::__isDeviceSuitable(VkPhysicalDevice vPhysicalDevice, VkSurfaceKHR vSurface,
                                        const std::vector<const char*>& vDeviceExtensions)
{
	SQueueFamilyIndices Indices;
	__findQueueFamilyIndices(Indices, vPhysicalDevice, vSurface);
	const bool IsQueueFamiliesComplete = Indices._GraphicsFamily.has_value() && Indices._PresentFamily.has_value();

	uint32_t ExtensionCount;
	vkEnumerateDeviceExtensionProperties(vPhysicalDevice, nullptr, &ExtensionCount, nullptr);
	std::vector<VkExtensionProperties> AvailableExtensions(ExtensionCount);
	vkEnumerateDeviceExtensionProperties(vPhysicalDevice, nullptr, &ExtensionCount, AvailableExtensions.data());
	std::set<std::string> RequiredExtensions(vDeviceExtensions.begin(), vDeviceExtensions.end());
	for (const auto& Extension : AvailableExtensions)
	{
		RequiredExtensions.erase(Extension.extensionName);
	}
	const bool IsExtensionsSupported = RequiredExtensions.empty();

	bool IsSwapChainAdequate = false;
	if (IsExtensionsSupported)
	{
		SSwapChainSupportDetails SwapChainSupportDetails;
		__querySwapChainSupportDetails(SwapChainSupportDetails, vPhysicalDevice, vSurface);
		IsSwapChainAdequate = !SwapChainSupportDetails._Formats.empty() && !SwapChainSupportDetails._PresentModes.empty();
	}

	return IsQueueFamiliesComplete && IsExtensionsSupported && IsSwapChainAdequate;
}

void Lemon::CDevice::__findQueueFamilyIndices(SQueueFamilyIndices& voQueueFamilyIndices,
	VkPhysicalDevice vPhysicalDevice, VkSurfaceKHR vSurface)
{
	uint32_t QueueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vPhysicalDevice, &QueueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(vPhysicalDevice, &QueueFamilyCount, QueueFamilies.data());
	int i = 0;
	for (const auto& QueueFamily : QueueFamilies)
	{
		if (QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			voQueueFamilyIndices._GraphicsFamily = i;
		VkBool32 IsPresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(vPhysicalDevice, i, vSurface, &IsPresentSupport);
		if (IsPresentSupport)
			voQueueFamilyIndices._PresentFamily = i;
		if (voQueueFamilyIndices._GraphicsFamily.has_value() && voQueueFamilyIndices._PresentFamily.has_value())
			break;
		i++;
	}
}

void Lemon::CDevice::__querySwapChainSupportDetails(SSwapChainSupportDetails& voDetails,
	VkPhysicalDevice vPhysicalDevice, VkSurfaceKHR vSurface)
{
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vPhysicalDevice, vSurface, &voDetails._Capabilities);

	uint32_t FormatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(vPhysicalDevice, vSurface, &FormatCount, nullptr);
	if (FormatCount != 0)
	{
		voDetails._Formats.resize(FormatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(vPhysicalDevice, vSurface, &FormatCount, voDetails._Formats.data());
	}

	uint32_t PresentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(vPhysicalDevice, vSurface, &PresentModeCount, nullptr);
	if (PresentModeCount != 0)
	{
		voDetails._PresentModes.resize(PresentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(vPhysicalDevice, vSurface, &PresentModeCount, voDetails._PresentModes.data());
	}
}
