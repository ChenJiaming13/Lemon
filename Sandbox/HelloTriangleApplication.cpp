#include "HelloTriangleApplication.h"
#include <set>
#include <limits>
#include <spdlog/spdlog.h>
#include "Shader.h"
#include "Transform.h"

void CHelloTriangleApplication::run()
{
	__initWindow();
	__initVulkan();
	__mainLoop();
	__cleanUp();
}

void CHelloTriangleApplication::__cleanUp()
{
	__cleanupSwapChain();
	vkDestroyBuffer(m_Device, m_VertexBuffer, nullptr);
	vkFreeMemory(m_Device, m_VertexDeviceMemory, nullptr);
	vkDestroyBuffer(m_Device, m_IndexBuffer, nullptr);
	vkFreeMemory(m_Device, m_IndexDeviceMemory, nullptr);
	for (int i = 0; i < m_MaxFramesInFlight; ++i)
	{
		vkDestroyBuffer(m_Device, m_UniformBuffers[i], nullptr);
		vkFreeMemory(m_Device, m_UniformDeviceMemories[i], nullptr);
	}
	vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
	for (int i = 0; i < m_MaxFramesInFlight; ++i)
	{
		vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
		vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
	}
	vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
	vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
	vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
	vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
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
		__drawFrame();
	}
}

void CHelloTriangleApplication::__initWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	m_pWindow = glfwCreateWindow(m_Width, m_Height, "Hello Triangle", nullptr, nullptr);
	if (m_pWindow != nullptr) spdlog::info("created window");
	glfwSetWindowUserPointer(m_pWindow, this);
	glfwSetFramebufferSizeCallback(m_pWindow, __framebufferResizeCallback);
}

void CHelloTriangleApplication::__initVulkan()
{
	__createInstance();
	__createSurface();
	__pickPhysicalDevice();
	__createLogicalDevice();
	__createSwapChain();
	__createImageViews();
	__createRenderPass();
	__createGraphicsPipeline();
	__createFramebuffers();
	__createCommandPool();
	__allocateCommandBuffers();
	__createSyncObjects();
	__createVertexBuffer();
	__createIndexBuffer();
	__createUniformBuffers();
	__createDescriptorPool();
	__createDescriptorSetLayout();
	__allocateDescriptorSets();
}

void CHelloTriangleApplication::__drawFrame()
{
	vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrFrame], VK_TRUE, UINT64_MAX);
	
	uint32_t ImageIndex;
	VkResult Result = vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrFrame], VK_NULL_HANDLE, &ImageIndex);
	if (Result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		__recreateSwapChain();
		return;
	}
	else if (Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR)
	{
		spdlog::error("failed to acquire swap chain image!");
	}

	vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrFrame]);

	__updateUniformBuffer(m_CurrFrame);
	vkResetCommandBuffer(m_CommandBuffers[m_CurrFrame], 0);
	__recordCommandBuffer(m_CommandBuffers[m_CurrFrame], ImageIndex);

	VkSubmitInfo SubmitInfo{};
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore WaitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrFrame]};
	VkPipelineStageFlags WaitFlags[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	SubmitInfo.waitSemaphoreCount = 1;
	SubmitInfo.pWaitSemaphores = WaitSemaphores;
	SubmitInfo.pWaitDstStageMask = WaitFlags;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &m_CommandBuffers[m_CurrFrame];
	VkSemaphore SignalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrFrame]};
	SubmitInfo.signalSemaphoreCount = 1;
	SubmitInfo.pSignalSemaphores = SignalSemaphores;
	if (vkQueueSubmit(m_GraphicsQueue, 1, &SubmitInfo, m_InFlightFences[m_CurrFrame]) != VK_SUCCESS)
	{
		spdlog::error("failed to submit draw command buffer");
	}

	VkPresentInfoKHR PresentInfo{};
	PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	PresentInfo.waitSemaphoreCount = 1;
	PresentInfo.pWaitSemaphores = SignalSemaphores;
	VkSwapchainKHR SwapChains[] = { m_SwapChain };
	PresentInfo.swapchainCount = 1;
	PresentInfo.pSwapchains = SwapChains;
	PresentInfo.pImageIndices = &ImageIndex;
	Result = vkQueuePresentKHR(m_PresentQueue, &PresentInfo);
	if (Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
	{
		m_FramebufferResized = false;
		__recreateSwapChain();
	}
	else if (Result != VK_SUCCESS)
	{
		spdlog::error("failed to present swap chain image!");
	}

	m_CurrFrame = (m_CurrFrame + 1) % m_MaxFramesInFlight;
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

void CHelloTriangleApplication::__cleanupSwapChain()
{
	for (const auto& Framebuffer : m_SwapChainFramebuffers)
	{
		vkDestroyFramebuffer(m_Device, Framebuffer, nullptr);
	}
	for (const auto& ImageView : m_SwapChainImageViews)
	{
		vkDestroyImageView(m_Device, ImageView, nullptr);
	}
	vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
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

void CHelloTriangleApplication::__recreateSwapChain()
{
	int Width = 0, Height = 0;
	glfwGetFramebufferSize(m_pWindow, &Width, &Height);
	while (Width == 0 || Height == 0)
	{
		glfwGetFramebufferSize(m_pWindow, &Width, &Height);
		glfwWaitEvents();
	}
	vkDeviceWaitIdle(m_Device);
	__cleanupSwapChain();
	__createSwapChain();
	__createImageViews();
	__createFramebuffers();
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

void CHelloTriangleApplication::__createShaderModule(const std::vector<char>& vShaderCode, VkShaderModule& voShaderModule) const
{
	VkShaderModuleCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	CreateInfo.codeSize = vShaderCode.size();
	CreateInfo.pCode = reinterpret_cast<const uint32_t*>(vShaderCode.data());
	if (vkCreateShaderModule(m_Device, &CreateInfo, nullptr, &voShaderModule) != VK_SUCCESS)
	{
		spdlog::error("failed to create shader module");
	}
	else spdlog::info("created shader module");
}

void CHelloTriangleApplication::__createRenderPass()
{
	VkAttachmentDescription AttachmentDescription{};
	AttachmentDescription.format = m_SwapChainImageFormat;
	AttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	AttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	AttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	AttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	AttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	AttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	AttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference AttachmentRef{};
	AttachmentRef.attachment = 0;
	AttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription SubpassDescription{};
	SubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	SubpassDescription.colorAttachmentCount = 1;
	SubpassDescription.pColorAttachments = &AttachmentRef;

	VkRenderPassCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	CreateInfo.attachmentCount = 1;
	CreateInfo.pAttachments = &AttachmentDescription;
	CreateInfo.subpassCount = 1;
	CreateInfo.pSubpasses = &SubpassDescription;

	VkSubpassDependency Dependency{};
	Dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	Dependency.dstSubpass = 0;
	Dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	Dependency.srcAccessMask = 0;
	Dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	Dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	CreateInfo.dependencyCount = 1;
	CreateInfo.pDependencies = &Dependency;

	if (vkCreateRenderPass(m_Device, &CreateInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
	{
		spdlog::error("failed to create render pass");
	}
	else spdlog::info("created render pass");
}

void CHelloTriangleApplication::__createDescriptorPool()
{
	VkDescriptorPoolSize PoolSize{};
	PoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	PoolSize.descriptorCount = static_cast<uint32_t>(m_MaxFramesInFlight);

	VkDescriptorPoolCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	CreateInfo.poolSizeCount = 1;
	CreateInfo.pPoolSizes = &PoolSize;
	CreateInfo.maxSets = static_cast<uint32_t>(m_MaxFramesInFlight);
	if (vkCreateDescriptorPool(m_Device, &CreateInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
	{
		spdlog::error("failed to create descriptor pool");
	}
	else spdlog::info("created descriptor pool");
}

void CHelloTriangleApplication::__createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding Binding{};
	Binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	Binding.binding = 0;
	Binding.descriptorCount = 1;
	Binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	Binding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	CreateInfo.bindingCount = 1;
	CreateInfo.pBindings = &Binding;
	if (vkCreateDescriptorSetLayout(m_Device, &CreateInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS)
	{
		spdlog::error("failed to create descriptor set layout");
	}
	else spdlog::info("created descriptor set layout");
}

void CHelloTriangleApplication::__allocateDescriptorSets()
{
	const std::vector Layouts(m_MaxFramesInFlight, m_DescriptorSetLayout);
	VkDescriptorSetAllocateInfo AllocateInfo{};
	AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	AllocateInfo.descriptorPool = m_DescriptorPool;
	AllocateInfo.descriptorSetCount = static_cast<uint32_t>(m_MaxFramesInFlight);
	AllocateInfo.pSetLayouts = Layouts.data();

	m_DescriptorSets.resize(m_MaxFramesInFlight);
	if (vkAllocateDescriptorSets(m_Device, &AllocateInfo, m_DescriptorSets.data()) != VK_SUCCESS)
	{
		spdlog::error("failed to allocate descriptor sets");
	}
	else spdlog::info("allocated descriptor sets");

	for (int i = 0; i < m_MaxFramesInFlight; ++i)
	{
		VkDescriptorBufferInfo BufferInfo{};
		BufferInfo.buffer = m_UniformBuffers[i];
		BufferInfo.offset = 0;
		BufferInfo.range = sizeof(SUniformBufferObject);

		VkWriteDescriptorSet WriteDescriptorSet{};
		WriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSet.dstSet = m_DescriptorSets[i];
		WriteDescriptorSet.dstBinding = 0;
		WriteDescriptorSet.dstArrayElement = 0;
		WriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		WriteDescriptorSet.descriptorCount = 1;
		WriteDescriptorSet.pBufferInfo = &BufferInfo;
		WriteDescriptorSet.pImageInfo = nullptr;
		WriteDescriptorSet.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(m_Device, 1, &WriteDescriptorSet, 0, nullptr);
		spdlog::info("updated descriptor set {}", i);
	}
}

void CHelloTriangleApplication::__createGraphicsPipeline()
{
	std::vector<char> VertShaderCode;
	std::vector<char> FragShaderCode;
	std::string Dir = "C:/Users/Chen/Documents/Code/Lemon/assets/shaders/";
	CShader::readFile(Dir + "vert2.spv", VertShaderCode);
	CShader::readFile(Dir + "frag2.spv", FragShaderCode);
	VkShaderModule VertShaderModule;
	VkShaderModule FragShaderModule;
	__createShaderModule(VertShaderCode, VertShaderModule);
	__createShaderModule(FragShaderCode, FragShaderModule);

	VkPipelineShaderStageCreateInfo VertShaderStageInfo{};
	VertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	VertShaderStageInfo.module = VertShaderModule;
	VertShaderStageInfo.pName = "main";
	VkPipelineShaderStageCreateInfo FragShaderStageInfo{};
	FragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	FragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	FragShaderStageInfo.module = FragShaderModule;
	FragShaderStageInfo.pName = "main";
	VkPipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo, FragShaderStageInfo };
	
	VkPipelineVertexInputStateCreateInfo VertexInputInfo{};
	VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VertexInputInfo.vertexBindingDescriptionCount = 1;
	auto BindingDescription = SVertex::getBindingDescription();
	VertexInputInfo.pVertexBindingDescriptions = &BindingDescription;
	VertexInputInfo.vertexAttributeDescriptionCount = 2;
	auto AttributeDescriptions = SVertex::getAttributeDescriptions();
	VertexInputInfo.pVertexAttributeDescriptions = AttributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo{};
	InputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	InputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	InputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo ViewportInfo{};
	ViewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	ViewportInfo.viewportCount = 1;
	ViewportInfo.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo RasterizationInfo{};
	RasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	RasterizationInfo.depthClampEnable = VK_FALSE;
	RasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
	RasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
	RasterizationInfo.lineWidth = 1.0f;
	RasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	RasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	RasterizationInfo.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo MultisampleInfo{};
	MultisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	MultisampleInfo.sampleShadingEnable = VK_FALSE;
	MultisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState ColorBlendAttachment{};
	ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT 
										| VK_COLOR_COMPONENT_G_BIT
										| VK_COLOR_COMPONENT_B_BIT 
										| VK_COLOR_COMPONENT_A_BIT;
	ColorBlendAttachment.blendEnable = VK_FALSE;
	VkPipelineColorBlendStateCreateInfo ColorBlendInfo{};
	ColorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	ColorBlendInfo.logicOpEnable = VK_FALSE;
	ColorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
	ColorBlendInfo.attachmentCount = 1;
	ColorBlendInfo.pAttachments = &ColorBlendAttachment;
	ColorBlendInfo.blendConstants[0] = 0.0f;
	ColorBlendInfo.blendConstants[1] = 0.0f;
	ColorBlendInfo.blendConstants[2] = 0.0f;
	ColorBlendInfo.blendConstants[3] = 0.0f;

	std::vector<VkDynamicState> DynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo DynamicInfo{};
	DynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	DynamicInfo.dynamicStateCount = (uint32_t)DynamicStates.size();
	DynamicInfo.pDynamicStates = DynamicStates.data();

	VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo{};
	__createDescriptorSetLayout();
	PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	PipelineLayoutCreateInfo.setLayoutCount = 1;
	PipelineLayoutCreateInfo.pSetLayouts = &m_DescriptorSetLayout;
	PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	if (vkCreatePipelineLayout(m_Device, &PipelineLayoutCreateInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
	{
		spdlog::error("failed to create pipeline layout");
	}
	else spdlog::info("created pipeline layout");

	// PIPELINE
	VkGraphicsPipelineCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	CreateInfo.stageCount = 2;
	CreateInfo.pStages = ShaderStages;
	CreateInfo.pVertexInputState = &VertexInputInfo;
	CreateInfo.pInputAssemblyState = &InputAssemblyInfo;
	CreateInfo.pViewportState = &ViewportInfo;
	CreateInfo.pRasterizationState = &RasterizationInfo;
	CreateInfo.pMultisampleState = &MultisampleInfo;
	CreateInfo.pColorBlendState = &ColorBlendInfo;
	CreateInfo.pDynamicState = &DynamicInfo;
	CreateInfo.layout = m_PipelineLayout;
	CreateInfo.renderPass = m_RenderPass;
	CreateInfo.subpass = 0;
	CreateInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &CreateInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
	{
		spdlog::error("failed to create graphics pipeline");
	}
	else spdlog::info("created graphics pipeline");

	vkDestroyShaderModule(m_Device, VertShaderModule, nullptr);
	vkDestroyShaderModule(m_Device, FragShaderModule, nullptr);
}

void CHelloTriangleApplication::__createFramebuffers()
{
	m_SwapChainFramebuffers.resize(m_SwapChainImageViews.size());
	for (size_t i = 0; i < m_SwapChainFramebuffers.size(); ++i)
	{
		VkImageView Attachments[] = { m_SwapChainImageViews[i] };
		VkFramebufferCreateInfo CreateInfo{};
		CreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		CreateInfo.renderPass = m_RenderPass;
		CreateInfo.attachmentCount = 1;
		CreateInfo.pAttachments = Attachments;
		CreateInfo.width = m_SwapChainExtent.width;
		CreateInfo.height = m_SwapChainExtent.height;
		CreateInfo.layers = 1;

		if (vkCreateFramebuffer(m_Device, &CreateInfo, nullptr, &m_SwapChainFramebuffers[i]) != VK_SUCCESS)
		{
			spdlog::error("failed to create framebuffer {}", i);
		}
		else spdlog::info("created framebuffer {}", i);
	}
}

void CHelloTriangleApplication::__createCommandPool()
{
	SRequiredQueueFamilyIndices Indices;
	__findQueueFamilies(m_PhysicalDevice, Indices);

	VkCommandPoolCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	CreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	CreateInfo.queueFamilyIndex = Indices._GraphicsFamily.value();

	if (vkCreateCommandPool(m_Device, &CreateInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
	{
		spdlog::error("failed to create command pool");
	}
	else spdlog::info("created command pool");
}

void CHelloTriangleApplication::__allocateCommandBuffers()
{
	m_CommandBuffers.resize(m_MaxFramesInFlight);
	VkCommandBufferAllocateInfo AllocateInfo{};
	AllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	AllocateInfo.commandPool = m_CommandPool;
	AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AllocateInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

	if (vkAllocateCommandBuffers(m_Device, &AllocateInfo, m_CommandBuffers.data()) != VK_SUCCESS)
	{
		spdlog::error("failed to allocate command buffer");
	}
	else spdlog::info("allocated command buffer");
}

void CHelloTriangleApplication::__recordCommandBuffer(const VkCommandBuffer& vCommandBuffer, uint32_t vImageIndex)
{
	VkCommandBufferBeginInfo BeginInfo{};
	BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (vkBeginCommandBuffer(vCommandBuffer, &BeginInfo) != VK_SUCCESS)
	{
		spdlog::error("failed to begin recording command buffer");
		return;
	}
	//else spdlog::info("recording command buffer");

	VkRenderPassBeginInfo RenderPassBeginInfo{};
	RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	RenderPassBeginInfo.renderPass = m_RenderPass;
	RenderPassBeginInfo.framebuffer = m_SwapChainFramebuffers[vImageIndex];
	RenderPassBeginInfo.renderArea.offset = { 0, 0 };
	RenderPassBeginInfo.renderArea.extent = m_SwapChainExtent;
	VkClearValue ClearColor = { {{ 0.0f, 0.0f, 0.0f, 1.0f}} };
	RenderPassBeginInfo.clearValueCount = 1;
	RenderPassBeginInfo.pClearValues = &ClearColor;
	vkCmdBeginRenderPass(vCommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(vCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
		VkViewport Viewport{};
		Viewport.x = 0.0f;
		Viewport.y = 0.0f;
		Viewport.width = (float)m_SwapChainExtent.width;
		Viewport.height = (float)m_SwapChainExtent.height;
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;
		vkCmdSetViewport(vCommandBuffer, 0, 1, &Viewport);
		VkRect2D Scissor{};
		Scissor.offset = { 0, 0 };
		Scissor.extent = m_SwapChainExtent;
		vkCmdSetScissor(vCommandBuffer, 0, 1, &Scissor);

		VkBuffer VertexBuffers[] = { m_VertexBuffer };
		VkDeviceSize Offsets[] = { 0 };
		vkCmdBindVertexBuffers(vCommandBuffer, 0, 1, VertexBuffers, Offsets);
		vkCmdBindIndexBuffer(vCommandBuffer, m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindDescriptorSets(vCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSets[m_CurrFrame], 0, nullptr);
		//vkCmdDraw(vCommandBuffer, (uint32_t)m_Vertices.size(), 1, 0, 0);
		vkCmdDrawIndexed(vCommandBuffer, (uint32_t)m_Indices.size(), 1, 0, 0, 0);

	vkCmdEndRenderPass(vCommandBuffer);

	if (vkEndCommandBuffer(vCommandBuffer) != VK_SUCCESS)
	{
		spdlog::error("failed to record command buffer");
	}
	//else spdlog::info("recorded command buffer");
}

void CHelloTriangleApplication::__createSyncObjects()
{
	m_ImageAvailableSemaphores.resize(m_MaxFramesInFlight);
	m_RenderFinishedSemaphores.resize(m_MaxFramesInFlight);
	m_InFlightFences.resize(m_MaxFramesInFlight);

	VkSemaphoreCreateInfo SemaphoreCreateInfo{};
	SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo FenceCreateInfo{};
	FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < m_MaxFramesInFlight; ++i)
	{
		if (vkCreateSemaphore(m_Device, &SemaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS
			|| vkCreateSemaphore(m_Device, &SemaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS
			|| vkCreateFence(m_Device, &FenceCreateInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS)
		{
			spdlog::error("failed to create synchronization objects for a frame");
		}
		else spdlog::info("created synchronization objects");
	}
}

void CHelloTriangleApplication::__createBuffer(VkDeviceSize vSize, VkBufferUsageFlags vUsageFlags, VkMemoryPropertyFlags vPropertyFlags, VkBuffer& voBuffer, VkDeviceMemory& voDeviceMemory)
{
	VkBufferCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	CreateInfo.size = vSize;
	CreateInfo.usage = vUsageFlags;
	CreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(m_Device, &CreateInfo, nullptr, &voBuffer) != VK_SUCCESS)
	{
		spdlog::error("failed to create buffer");
		return;
	}
	else spdlog::info("created buffer");
	VkMemoryRequirements MemoryRequirements;
	vkGetBufferMemoryRequirements(m_Device, voBuffer, &MemoryRequirements);
	VkMemoryAllocateInfo AllocateInfo{};
	AllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	AllocateInfo.allocationSize = MemoryRequirements.size;
	AllocateInfo.memoryTypeIndex = __findMemoryType(MemoryRequirements.memoryTypeBits, vPropertyFlags);
	if (vkAllocateMemory(m_Device, &AllocateInfo, nullptr, &voDeviceMemory) != VK_SUCCESS)
	{
		spdlog::error("failed to allocate device memory");
		return;
	}
	else spdlog::info("allocated device memory");
	vkBindBufferMemory(m_Device, voBuffer, voDeviceMemory, 0);
}

void CHelloTriangleApplication::__copyBuffer(VkBuffer vSrcBuffer, VkBuffer vDstBuffer, VkDeviceSize vBufferSize)
{
	VkCommandBufferAllocateInfo AllocateInfo{};
	AllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	AllocateInfo.commandPool = m_CommandPool;
	AllocateInfo.commandBufferCount = 1;
	AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	VkCommandBuffer CommandBuffer;
	vkAllocateCommandBuffers(m_Device, &AllocateInfo, &CommandBuffer);

	VkCommandBufferBeginInfo BeginInfo{};
	BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(CommandBuffer, &BeginInfo);
	
	VkBufferCopy CopyRegion{};
	CopyRegion.size = vBufferSize;
	CopyRegion.srcOffset = 0;
	CopyRegion.dstOffset = 0;
	vkCmdCopyBuffer(CommandBuffer, vSrcBuffer, vDstBuffer, 1, &CopyRegion);

	vkEndCommandBuffer(CommandBuffer);

	VkSubmitInfo SubmitInfo{};
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &CommandBuffer;
	vkQueueSubmit(m_GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_GraphicsQueue);

	vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &CommandBuffer);
}

void CHelloTriangleApplication::__createVertexBuffer()
{
	m_Vertices = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};
	VkDeviceSize BufferSize = sizeof(m_Vertices[0]) * m_Vertices.size();

	VkBuffer StagingBuffer;
	VkDeviceMemory StagingDeviceMemory;
	__createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer, StagingDeviceMemory);

	void* pData;
	vkMapMemory(m_Device, StagingDeviceMemory, 0, BufferSize, 0, &pData);
	memcpy(pData, m_Vertices.data(), BufferSize);
	vkUnmapMemory(m_Device, StagingDeviceMemory);

	__createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_VertexDeviceMemory);

	__copyBuffer(StagingBuffer, m_VertexBuffer, BufferSize);
	vkDestroyBuffer(m_Device, StagingBuffer, nullptr);
	vkFreeMemory(m_Device, StagingDeviceMemory, nullptr);
}

void CHelloTriangleApplication::__createIndexBuffer()
{
	m_Indices = {
		0, 1, 2,
		2, 3, 0
	};
	VkDeviceSize BufferSize = sizeof(m_Indices[0]) * m_Indices.size();

	VkBuffer StagingBuffer;
	VkDeviceMemory StagingDeviceMemory;
	__createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer, StagingDeviceMemory);

	void* pData;
	vkMapMemory(m_Device, StagingDeviceMemory, 0, BufferSize, 0, &pData);
	memcpy(pData, m_Indices.data(), BufferSize);
	vkUnmapMemory(m_Device, StagingDeviceMemory);

	__createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBuffer, m_IndexDeviceMemory);

	__copyBuffer(StagingBuffer, m_IndexBuffer, BufferSize);
	vkDestroyBuffer(m_Device, StagingBuffer, nullptr);
	vkFreeMemory(m_Device, StagingDeviceMemory, nullptr);
}

void CHelloTriangleApplication::__createUniformBuffers()
{
	VkDeviceSize BufferSize = sizeof(SUniformBufferObject);
	m_UniformBuffers.resize(m_MaxFramesInFlight);
	m_UniformDeviceMemories.resize(m_MaxFramesInFlight);
	m_UniformMappedPointers.resize(m_MaxFramesInFlight);
	for (size_t i = 0; i < m_MaxFramesInFlight; ++i)
	{
		__createBuffer(
			BufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_UniformBuffers[i], m_UniformDeviceMemories[i]);
		vkMapMemory(m_Device, m_UniformDeviceMemories[i], 0, BufferSize, 0, &m_UniformMappedPointers[i]);
	}
}

void CHelloTriangleApplication::__updateUniformBuffer(uint32_t vImageIndex)
{
	SUniformBufferObject UBO;
	CTransform::dumpUniformBufferObject(m_SwapChainExtent.width, m_SwapChainExtent.height, UBO);
	memcpy(m_UniformMappedPointers[vImageIndex], &UBO, sizeof(UBO));
}

uint32_t CHelloTriangleApplication::__findMemoryType(uint32_t vTypeFilter, VkMemoryPropertyFlags vFlags)
{
	VkPhysicalDeviceMemoryProperties MemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &MemoryProperties);
	for (uint32_t i = 0; i < MemoryProperties.memoryTypeCount; ++i)
	{
		if ((vTypeFilter & (1 << i)) &&
			(MemoryProperties.memoryTypes[i].propertyFlags & vFlags) == vFlags)
		{
			return i;
		}
	}
	spdlog::error("failed to find suitable memory type");
	return 0;
}

void CHelloTriangleApplication::__framebufferResizeCallback(GLFWwindow* vWindow, int vWidth, int vHeight)
{
	auto pApp = (CHelloTriangleApplication*)glfwGetWindowUserPointer(vWindow);
	pApp->m_FramebufferResized = true;
}