#include "HelloTriangleApplication.h"
#include <set>
#include <limits>
#include <spdlog/spdlog.h>

void CHelloTriangleApplication::run()
{
	__initWindow();
	__initVulkan();
	__mainLoop();
	__cleanUp();
}

void CHelloTriangleApplication::__cleanUp()
{
	for (const auto& ImageView : m_SwapChainImageViews)
	{
		vkDestroyImageView(m_Device, ImageView, nullptr);
	}
	vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
	vkDestroyDevice(m_Device, nullptr);
	vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
	vkDestroyInstance(m_Instance, nullptr);
	glfwDestroyWindow(m_pWindow);
	glfwTerminate();
}

void CHelloTriangleApplication::__mainLoop()
{
	while (!glfwWindowShouldClose(m_pWindow))
	{
		glfwPollEvents();
	}
}

void CHelloTriangleApplication::__initWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_pWindow = glfwCreateWindow(m_Width, m_Height, "Hello Triangle", nullptr, nullptr);
	if (m_pWindow != nullptr)
		spdlog::info("created window");
}

void CHelloTriangleApplication::__initVulkan()
{
	__createInstance();
	__createSurface();
	__pickPhysicalDevice();
	__createLogicalDevice();
	__createSwapChain();
	__createImageViews();
}

void CHelloTriangleApplication::__setRequiredInstanceExtensions()
{
	m_RequiredInstanceExtensions.clear();
	uint32_t GlfwExtensionCount = 0;
	const char **GlfwExtensions = glfwGetRequiredInstanceExtensions(&GlfwExtensionCount);
	for (uint32_t i = 0; i < GlfwExtensionCount; ++i)
	{
		m_RequiredInstanceExtensions.push_back(GlfwExtensions[i]);
		spdlog::trace("required instance extension: {}", m_RequiredInstanceExtensions.back());
	}
}

void CHelloTriangleApplication::__setRequiredPhyDeviceExtensions()
{
	m_RequiredPhyDeviceExtensions.clear();
	m_RequiredPhyDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

void CHelloTriangleApplication::__createInstance()
{
	VkApplicationInfo AppInfo{};
	AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	AppInfo.pApplicationName = "Hello Triangle";
	AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	AppInfo.pEngineName = "No Engine";
	AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	AppInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	CreateInfo.pApplicationInfo = &AppInfo;
	__setRequiredInstanceExtensions();
	CreateInfo.enabledExtensionCount = (uint32_t)m_RequiredInstanceExtensions.size();
	CreateInfo.ppEnabledExtensionNames = m_RequiredInstanceExtensions.data();
	CreateInfo.enabledLayerCount = 0;
	CreateInfo.ppEnabledLayerNames = nullptr;
	CreateInfo.pNext = nullptr;

	if (vkCreateInstance(&CreateInfo, nullptr, &m_Instance) != VK_SUCCESS)
		spdlog::error("failed to create instance!");
	else
		spdlog::info("created instance");
}

void CHelloTriangleApplication::__createSurface()
{
	if (glfwCreateWindowSurface(m_Instance, m_pWindow, nullptr, &m_Surface) != VK_SUCCESS)
	{
		spdlog::error("failed to create window surface!");
	}
	else spdlog::info("created window surface");
}

void CHelloTriangleApplication::__findQueueFamilies(const VkPhysicalDevice &vPhyDevice, SRequiredQueueFamilyIndices &voFamilyIndices) const
{
	uint32_t QueueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vPhyDevice, &QueueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(vPhyDevice, &QueueFamilyCount, QueueFamilies.data());
	for (uint32_t i = 0; i < QueueFamilyCount; ++i)
	{
		if (QueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			voFamilyIndices._GraphicsFamily = i;

		VkBool32 PresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(vPhyDevice, i, m_Surface, &PresentSupport);
		if (PresentSupport)
			voFamilyIndices._PresentFamily = i;

		if (voFamilyIndices.isComplete())
			break;
	}
}

void CHelloTriangleApplication::__querySwapChainSupport(const VkPhysicalDevice& vPhyDevice, SSwapChainSupportDetails& voDetails) const
{
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vPhyDevice, m_Surface, &voDetails._Capabilities);
	uint32_t FormatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(vPhyDevice, m_Surface, &FormatCount, nullptr);
	if (FormatCount != 0)
	{
		voDetails._Formats.resize(FormatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(vPhyDevice, m_Surface, &FormatCount, voDetails._Formats.data());
	}
	uint32_t PresentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(vPhyDevice, m_Surface, &PresentModeCount, nullptr);
	if (PresentModeCount != 0)
	{
		voDetails._PresentModes.resize(PresentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(vPhyDevice, m_Surface, &PresentModeCount, voDetails._PresentModes.data());
	}
}

bool CHelloTriangleApplication::__isPhysicalDeviceSuitable(VkPhysicalDevice vPhyDevice)
{
	SRequiredQueueFamilyIndices Indices;
	__findQueueFamilies(vPhyDevice, Indices);

	// note! set<string> instead of std<const char*>
	std::set<std::string> RequiredPhyDeviceExtensions(m_RequiredPhyDeviceExtensions.begin(), m_RequiredPhyDeviceExtensions.end());
	uint32_t AvailableExtensionCount;
	vkEnumerateDeviceExtensionProperties(vPhyDevice, nullptr, &AvailableExtensionCount, nullptr);
	std::vector<VkExtensionProperties> AvailableExtensions(AvailableExtensionCount);
	vkEnumerateDeviceExtensionProperties(vPhyDevice, nullptr, &AvailableExtensionCount, AvailableExtensions.data());

	for (const auto& Extension : AvailableExtensions)
	{
		RequiredPhyDeviceExtensions.erase(Extension.extensionName);
	}

	bool IsExtensionsSupported = RequiredPhyDeviceExtensions.empty();
	bool IsSwapChainAdequate = false;
	if (IsExtensionsSupported)
	{
		SSwapChainSupportDetails Details;
		__querySwapChainSupport(vPhyDevice, Details);
		IsSwapChainAdequate = !Details._Formats.empty() && !Details._PresentModes.empty();
	}

	return Indices.isComplete() && IsExtensionsSupported && IsSwapChainAdequate;
}

void CHelloTriangleApplication::__pickPhysicalDevice()
{
	__setRequiredPhyDeviceExtensions();
	uint32_t PhyDeviceCount = 0;
	vkEnumeratePhysicalDevices(m_Instance, &PhyDeviceCount, nullptr);
	if (PhyDeviceCount == 0)
		spdlog::error("failed to find GPUs with vulkan support!");
	std::vector<VkPhysicalDevice> PhyDevices(PhyDeviceCount);
	vkEnumeratePhysicalDevices(m_Instance, &PhyDeviceCount, PhyDevices.data());
	for (const auto& PhyDevice : PhyDevices)
	{
		VkPhysicalDeviceProperties PhyDeviceProperties;
		vkGetPhysicalDeviceProperties(PhyDevice, &PhyDeviceProperties);
		spdlog::trace("detected physical device: {}", PhyDeviceProperties.deviceName);
	}
	for (const auto &PhyDevice: PhyDevices)
	{
		if (__isPhysicalDeviceSuitable(PhyDevice))
		{
			m_PhysicalDevice = PhyDevice;
			VkPhysicalDeviceProperties PhyDeviceProperties;
			vkGetPhysicalDeviceProperties(PhyDevice, &PhyDeviceProperties);
			spdlog::info("picked physical device: {}", PhyDeviceProperties.deviceName);
			break;
		}
	}
	if (m_PhysicalDevice == VK_NULL_HANDLE)
		spdlog::error("failed to find a suitable GPU!");
}

void CHelloTriangleApplication::__createLogicalDevice()
{
	SRequiredQueueFamilyIndices Indices;
	__findQueueFamilies(m_PhysicalDevice, Indices);
	std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
	std::set<uint32_t> UniqueQueueFamilies = { Indices._GraphicsFamily.value(), Indices._PresentFamily.value() };
	for (uint32_t QueueFamily : UniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo QueueCreateInfo{};
		QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		QueueCreateInfo.queueFamilyIndex = QueueFamily;
		QueueCreateInfo.queueCount = 1;
		float QueuePriority = 1.0f;
		QueueCreateInfo.pQueuePriorities = &QueuePriority;
		QueueCreateInfos.push_back(QueueCreateInfo);
	}

	VkPhysicalDeviceFeatures PhyDeviceFeatures{};
	
	VkDeviceCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	CreateInfo.queueCreateInfoCount = (uint32_t)QueueCreateInfos.size();
	CreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
	CreateInfo.pEnabledFeatures = &PhyDeviceFeatures;
	CreateInfo.enabledExtensionCount = (uint32_t)m_RequiredPhyDeviceExtensions.size();
	CreateInfo.ppEnabledExtensionNames = m_RequiredPhyDeviceExtensions.data();
	CreateInfo.enabledLayerCount = 0;

	if (vkCreateDevice(m_PhysicalDevice, &CreateInfo, nullptr, &m_Device) != VK_SUCCESS)
	{
		spdlog::error("failed to create logical device!");
	}
	else spdlog::info("created logical device");

	vkGetDeviceQueue(m_Device, Indices._GraphicsFamily.value(), 0, &m_GraphicsQueue);
	spdlog::info("get a graphics queue ({})", Indices._GraphicsFamily.value());
	vkGetDeviceQueue(m_Device, Indices._PresentFamily.value(), 0, &m_PresentQueue);
	spdlog::info("get a present queue ({})", Indices._PresentFamily.value());
}

void CHelloTriangleApplication::__createSwapChain()
{
	SSwapChainSupportDetails Details;
	__querySwapChainSupport(m_PhysicalDevice, Details);

	// choose format
	VkSurfaceFormatKHR Format = Details._Formats[0];
	for (const auto& AvailableFormat : Details._Formats)
	{
		if (AvailableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && AvailableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			Format = AvailableFormat;
			break;
		}
	}

	// choose present mode
	VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (const auto& AvailablePresentMode : Details._PresentModes)
	{
		if (AvailablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			PresentMode = AvailablePresentMode;
			break;
		}
	}

	// choose swap extent
	VkExtent2D Extent = Details._Capabilities.currentExtent;
	if (Details._Capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
	{
		int Width, Height;
		glfwGetFramebufferSize(m_pWindow, &Width, &Height);
		Extent.width = std::clamp(
			(uint32_t)Width, 
			Details._Capabilities.minImageExtent.width, 
			Details._Capabilities.maxImageExtent.width
		);
		Extent.height = std::clamp(
			(uint32_t)Height, 
			Details._Capabilities.minImageExtent.height, 
			Details._Capabilities.maxImageExtent.height
		);
	}
	
	// choose image count
	uint32_t ImageCount = Details._Capabilities.minImageCount + 1;
	if (Details._Capabilities.maxImageCount > 0 && ImageCount > Details._Capabilities.maxImageCount)
	{
		ImageCount = Details._Capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	CreateInfo.surface = m_Surface;
	CreateInfo.minImageCount = ImageCount;
	CreateInfo.imageFormat = Format.format;
	CreateInfo.imageColorSpace = Format.colorSpace;
	CreateInfo.imageExtent = Extent;
	CreateInfo.imageArrayLayers = 1;
	CreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	
	SRequiredQueueFamilyIndices Indices;
	__findQueueFamilies(m_PhysicalDevice, Indices);
	uint32_t QueueFamilyIndices[] = { Indices._GraphicsFamily.value(), Indices._PresentFamily.value() };
	if (Indices._GraphicsFamily != Indices._PresentFamily)
	{
		CreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		CreateInfo.queueFamilyIndexCount = 2;
		CreateInfo.pQueueFamilyIndices = QueueFamilyIndices;
	}
	else
	{
		CreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		CreateInfo.queueFamilyIndexCount = 0;
		CreateInfo.pQueueFamilyIndices = nullptr;
	}
	
	CreateInfo.preTransform = Details._Capabilities.currentTransform;
	CreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	CreateInfo.presentMode = PresentMode;
	CreateInfo.clipped = VK_TRUE;
	CreateInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(m_Device, &CreateInfo, nullptr, &m_SwapChain) != VK_SUCCESS)
	{
		spdlog::error("failed to create swap chain!");
		return;
	}
	else spdlog::info("created swap chain");
	uint32_t SwapChainImageCount;
	vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &SwapChainImageCount, nullptr);
	m_SwapChainImages.resize(ImageCount);
	vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &SwapChainImageCount, m_SwapChainImages.data());
	m_SwapChainImageFormat = Format.format;
	m_SwapChainExtent = Extent;
	spdlog::info("get swap chain images");
}

void CHelloTriangleApplication::__createImageViews()
{
	m_SwapChainImageViews.resize(m_SwapChainImages.size());
	for (size_t i = 0; i < m_SwapChainImageViews.size(); ++i)
	{
		VkImageViewCreateInfo CreateInfo{};
		CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		CreateInfo.image = m_SwapChainImages[i];
		CreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		CreateInfo.format = m_SwapChainImageFormat;
		CreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		CreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		CreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		CreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		CreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		CreateInfo.subresourceRange.baseMipLevel = 0;
		CreateInfo.subresourceRange.levelCount = 1;
		CreateInfo.subresourceRange.baseArrayLayer = 0;
		CreateInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(m_Device, &CreateInfo, nullptr, &m_SwapChainImageViews[i]) != VK_SUCCESS)
		{
			spdlog::error("failed to create image view {}", i);
		}
		else spdlog::info("created image view {}", i);
	}
}
