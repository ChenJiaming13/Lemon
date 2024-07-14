#include "HelloTriangleApplication.h"
#include <spdlog/spdlog.h>

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

void CHelloTriangleApplication::run()
{
	__initWindow();
	__initVulkan();
	__mainLoop();
	__cleanUp();
}

void CHelloTriangleApplication::__cleanUp()
{
	vkDestroyDevice(m_Device, nullptr);
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
	m_pWindow = glfwCreateWindow(WIDTH, HEIGHT, "Hello Triangle", nullptr, nullptr);
	if (m_pWindow != nullptr)
		spdlog::info("created window");
}

void CHelloTriangleApplication::__initVulkan()
{
	__createInstance();
	__pickPhysicalDevice();
	__createLogicalDevice();
}

void CHelloTriangleApplication::__dumpRequiredExtensions(std::vector<const char *> &voExtensions)
{
	uint32_t GlfwExtensionCount = 0;
	const char **GlfwExtensions = glfwGetRequiredInstanceExtensions(&GlfwExtensionCount);
	for (uint32_t i = 0; i < GlfwExtensionCount; ++i)
	{
		voExtensions.push_back(GlfwExtensions[i]);
		spdlog::info("instance extension: {}", voExtensions.back());
	}
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
	std::vector<const char *> Extensions;
	__dumpRequiredExtensions(Extensions);
	CreateInfo.enabledExtensionCount = (uint32_t)Extensions.size();
	CreateInfo.ppEnabledExtensionNames = Extensions.data();
	CreateInfo.enabledLayerCount = 0;
	CreateInfo.ppEnabledLayerNames = nullptr;
	CreateInfo.pNext = nullptr;

	if (vkCreateInstance(&CreateInfo, nullptr, &m_Instance) != VK_SUCCESS)
		spdlog::error("failed to create instance!");
	else
		spdlog::info("created instance");
}

void CHelloTriangleApplication::__findQueueFamilies(const VkPhysicalDevice &vPhyDevice, SRequiredQueueFamilyIndices &voFamilyIndices)
{
	uint32_t QueueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vPhyDevice, &QueueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(vPhyDevice, &QueueFamilyCount, QueueFamilies.data());
	for (uint32_t i = 0; i < QueueFamilyCount; ++i)
	{
		if (QueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			voFamilyIndices._GraphicsFamily = i;
		if (voFamilyIndices.isComplete())
			break;
	}
}

void CHelloTriangleApplication::__pickPhysicalDevice()
{
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
		SRequiredQueueFamilyIndices QueueFamilyIndices;
		__findQueueFamilies(PhyDevice, QueueFamilyIndices);
		if (QueueFamilyIndices.isComplete())
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
	VkDeviceQueueCreateInfo QueueCreateInfo{};
	QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	QueueCreateInfo.queueFamilyIndex = Indices._GraphicsFamily.value();
	QueueCreateInfo.queueCount = 1;
	float QueuePriority = 1.0f;
	QueueCreateInfo.pQueuePriorities = &QueuePriority;

	VkPhysicalDeviceFeatures PhyDeviceFeatures{};
	
	VkDeviceCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	CreateInfo.pQueueCreateInfos = &QueueCreateInfo;
	CreateInfo.queueCreateInfoCount = 1;
	CreateInfo.pEnabledFeatures = &PhyDeviceFeatures;
	CreateInfo.enabledExtensionCount = 0;
	CreateInfo.enabledLayerCount = 0;

	if (vkCreateDevice(m_PhysicalDevice, &CreateInfo, nullptr, &m_Device) != VK_SUCCESS)
	{
		spdlog::error("failed to create logical device!");
	}
	else spdlog::info("created logical device");

	vkGetDeviceQueue(m_Device, Indices._GraphicsFamily.value(), 0, &m_GraphicsQueue);
}