#include "pch.h"
#include "core.h"

GLFWwindow* Lemon::createWindow(int vWidth, int vHeight, const std::string& vTitle)
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	const auto pWindow = glfwCreateWindow(vWidth, vHeight, vTitle.c_str(), nullptr, nullptr);
	if (pWindow == nullptr)
	{
		spdlog::error("failed to create window");
		glfwTerminate();
		return nullptr;
	}
	return pWindow;
}

bool Lemon::createInstance(VkInstance& voInstance, const std::vector<const char*>& vInstanceExtensions)
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
	CreateInfo.enabledExtensionCount = static_cast<uint32_t>(vInstanceExtensions.size());
	CreateInfo.ppEnabledExtensionNames = vInstanceExtensions.data();
	CreateInfo.enabledLayerCount = 0;
	CreateInfo.pNext = nullptr;

	if (vkCreateInstance(&CreateInfo, nullptr, &voInstance) != VK_SUCCESS)
	{
		spdlog::error("failed to create instance!");
		return false;
	}
	return true;
}

bool Lemon::createSurface(VkSurfaceKHR& voSurface, const VkInstance& vInstance, GLFWwindow* vWindow)
{
	if (glfwCreateWindowSurface(vInstance, vWindow, nullptr, &voSurface) != VK_SUCCESS)
	{
		spdlog::error("failed to create window surface!");
		return false;
	}
	return true;
}

bool Lemon::pickPhysicalDevice(VkPhysicalDevice& voPhysicalDevice, const VkInstance& vInstance, const VkSurfaceKHR& vSurface, const std::vector<const char*>& vDeviceExtensions)
{
	voPhysicalDevice = VK_NULL_HANDLE;

	uint32_t PhysicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(vInstance, &PhysicalDeviceCount, nullptr);
	if (PhysicalDeviceCount == 0)
	{
		spdlog::error("failed to find GPUs with Vulkan support!");
		return false;
	}
	std::vector<VkPhysicalDevice> PhysicalDevices(PhysicalDeviceCount);
	vkEnumeratePhysicalDevices(vInstance, &PhysicalDeviceCount, PhysicalDevices.data());
	for (const auto& PhysicalDevice : PhysicalDevices)
	{
		if (isDeviceSuitable(PhysicalDevice, vSurface, vDeviceExtensions))
		{
			voPhysicalDevice = PhysicalDevice;
			break;
		}
	}
	if (voPhysicalDevice == VK_NULL_HANDLE)
	{
		spdlog::error("failed to find a suitable GPU!");
		return false;
	}
	return true;
}

bool Lemon::createLogicalDeviceAndQueues(VkDevice& voDevice, VkQueue& voGraphicsQueue, VkQueue& voPresentQueue,
	const VkPhysicalDevice& vPhysicalDevice, const VkSurfaceKHR& vSurface,
	const std::vector<const char*>& vDeviceExtensions)
{
	SQueueFamilyIndices Indices;
	findQueueFamilyIndices(Indices, vPhysicalDevice, vSurface);
	if (!Indices._GraphicsFamily.has_value() || !Indices._PresentFamily.has_value())
	{
		spdlog::error("SQueueFamilyIndices is not complete!");
		return false;
	}
	std::set UniqueQueueFamilyIndices = { Indices._GraphicsFamily.value(), Indices._PresentFamily.value() };

	std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
	float QueuePriority = 1.0f;
	for (uint32_t QueueFamilyIndex : UniqueQueueFamilyIndices)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = QueueFamilyIndex;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &QueuePriority;
		QueueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures DeviceFeatures{};

	VkDeviceCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	CreateInfo.queueCreateInfoCount = static_cast<uint32_t>(QueueCreateInfos.size());
	CreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
	CreateInfo.pEnabledFeatures = &DeviceFeatures;
	CreateInfo.enabledExtensionCount = static_cast<uint32_t>(vDeviceExtensions.size());
	CreateInfo.ppEnabledExtensionNames = vDeviceExtensions.data();
	CreateInfo.enabledLayerCount = 0;

	if (vkCreateDevice(vPhysicalDevice, &CreateInfo, nullptr, &voDevice) != VK_SUCCESS)
	{
		spdlog::error("failed to create logical device!");
		return false;
	}

	vkGetDeviceQueue(voDevice, Indices._GraphicsFamily.value(), 0, &voGraphicsQueue);
	vkGetDeviceQueue(voDevice, Indices._PresentFamily.value(), 0, &voPresentQueue);

	return true;
}

bool Lemon::createCommandPool(VkCommandPool& voCommandPool, const VkDevice& vDevice,
	const VkPhysicalDevice& vPhysicalDevice, const VkSurfaceKHR& vSurface)
{
	SQueueFamilyIndices Indices;
	findQueueFamilyIndices(Indices, vPhysicalDevice, vSurface);
	if (!Indices._GraphicsFamily.has_value())
	{
		spdlog::error("SQueueFamilyIndices is not complete!");
		return false;
	}

	VkCommandPoolCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	CreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	CreateInfo.queueFamilyIndex = Indices._GraphicsFamily.value();

	if (vkCreateCommandPool(vDevice, &CreateInfo, nullptr, &voCommandPool) != VK_SUCCESS)
	{
		spdlog::error("failed to create graphics command pool!");
		return false;
	}
	return true;
}

bool Lemon::allocateCommandBuffers(std::vector<VkCommandBuffer>& voCommandBuffers, const VkDevice& vDevice,
	const VkCommandPool& vCommandPool, size_t vAllocateCount)
{
	voCommandBuffers.resize(vAllocateCount);

	VkCommandBufferAllocateInfo AllocateInfo{};
	AllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	AllocateInfo.commandPool = vCommandPool;
	AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AllocateInfo.commandBufferCount = static_cast<uint32_t>(voCommandBuffers.size());

	if (vkAllocateCommandBuffers(vDevice, &AllocateInfo, voCommandBuffers.data()) != VK_SUCCESS)
	{
		spdlog::error("failed to allocate command buffers!");
		return false;
	}
	return true;
}

bool Lemon::createSwapChain(VkSwapchainKHR& voSwapChain, VkFormat& voSwapChainImageFormat,
							VkExtent2D& voSwapChainExtent, const VkDevice& vDevice, const VkPhysicalDevice& vPhysicalDevice, const VkSurfaceKHR& vSurface, int vWidth, int vHeight)
{
	SSwapChainSupportDetails Details;
	querySwapChainSupportDetails(Details, vPhysicalDevice, vSurface);

	if (Details._Formats.empty())
	{
		spdlog::error("swapchain supported surface format is empty!");
		return false;
	}
	VkSurfaceFormatKHR SurfaceFormat = Details._Formats[0];
	for (const auto& AvailableFormat : Details._Formats)
	{
		if (AvailableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && AvailableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			SurfaceFormat = AvailableFormat;
			break;
		}
	}

	VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (const auto& AvailablePresentMode : Details._PresentModes)
	{
		if (AvailablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			PresentMode = AvailablePresentMode;
		}
	}

	if (Details._Capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		voSwapChainExtent = Details._Capabilities.currentExtent;
	}
	else
	{
		const VkExtent2D ActualExtent = { static_cast<uint32_t>(vWidth), static_cast<uint32_t>(vHeight) };
		voSwapChainExtent.width = std::clamp(ActualExtent.width, Details._Capabilities.minImageExtent.width, Details._Capabilities.maxImageExtent.width);
		voSwapChainExtent.height = std::clamp(ActualExtent.height, Details._Capabilities.minImageExtent.height, Details._Capabilities.maxImageExtent.height);
	}

	uint32_t ImageCount = Details._Capabilities.minImageCount + 1;
	if (Details._Capabilities.maxImageCount > 0 && ImageCount > Details._Capabilities.maxImageCount) {
		ImageCount = Details._Capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	CreateInfo.surface = vSurface;
	CreateInfo.minImageCount = ImageCount;
	CreateInfo.imageFormat = SurfaceFormat.format;
	CreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
	CreateInfo.imageExtent = voSwapChainExtent;
	CreateInfo.imageArrayLayers = 1;
	CreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	SQueueFamilyIndices Indices;
	findQueueFamilyIndices(Indices, vPhysicalDevice, vSurface);
	if (!Indices._GraphicsFamily.has_value() || !Indices._PresentFamily.has_value())
	{
		spdlog::error("SQueueFamilyIndices is not complete!");
		return false;
	}
	const uint32_t QueueFamilyIndices[] = { Indices._GraphicsFamily.value(), Indices._PresentFamily.value() };

	if (Indices._GraphicsFamily != Indices._PresentFamily)
	{
		CreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		CreateInfo.queueFamilyIndexCount = 2;
		CreateInfo.pQueueFamilyIndices = QueueFamilyIndices;
	}
	else
	{
		CreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	CreateInfo.preTransform = Details._Capabilities.currentTransform;
	CreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	CreateInfo.presentMode = PresentMode;
	CreateInfo.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(vDevice, &CreateInfo, nullptr, &voSwapChain) != VK_SUCCESS)
	{
		spdlog::error("failed to create swap chain!");
		return false;
	}

	voSwapChainImageFormat = SurfaceFormat.format;
	return true;
}

void Lemon::getSwapChainImages(std::vector<VkImage>& voSwapChainImages, const VkDevice& vDevice,
	const VkSwapchainKHR& vSwapchain)
{
	uint32_t ImageCount;
	vkGetSwapchainImagesKHR(vDevice, vSwapchain, &ImageCount, nullptr);
	voSwapChainImages.resize(ImageCount);
	vkGetSwapchainImagesKHR(vDevice, vSwapchain, &ImageCount, voSwapChainImages.data());
}

bool Lemon::createImageViews(std::vector<VkImageView>& voSwapChainImageViews, const VkDevice& vDevice,
	const std::vector<VkImage>& vSwapChainImages, const VkFormat& vSwapChainImageFormat)
{
	voSwapChainImageViews.resize(vSwapChainImages.size());
	for (size_t i = 0; i < vSwapChainImages.size(); i++)
	{
		VkImageViewCreateInfo CreateInfo{};
		CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		CreateInfo.image = vSwapChainImages[i];
		CreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		CreateInfo.format = vSwapChainImageFormat;
		CreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		CreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		CreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		CreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		CreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		CreateInfo.subresourceRange.baseMipLevel = 0;
		CreateInfo.subresourceRange.levelCount = 1;
		CreateInfo.subresourceRange.baseArrayLayer = 0;
		CreateInfo.subresourceRange.layerCount = 1;
		if (vkCreateImageView(vDevice, &CreateInfo, nullptr, &voSwapChainImageViews[i]) != VK_SUCCESS)
		{
			spdlog::error("failed to create image views {}!", i);
			return false;
		}
	}
	return true;
}

bool Lemon::createRenderPass(VkRenderPass& voRenderPass, const VkDevice& vDevice, const VkFormat& vSwapChainImageFormat)
{
	VkAttachmentDescription ColorAttachment{};
	ColorAttachment.format = vSwapChainImageFormat;
	ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference ColorAttachmentRef;
	ColorAttachmentRef.attachment = 0;
	ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription SubpassDescription{};
	SubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	SubpassDescription.colorAttachmentCount = 1;
	SubpassDescription.pColorAttachments = &ColorAttachmentRef;

	VkSubpassDependency Dependency{};
	Dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	Dependency.dstSubpass = 0;
	Dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	Dependency.srcAccessMask = 0;
	Dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	Dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	CreateInfo.attachmentCount = 1;
	CreateInfo.pAttachments = &ColorAttachment;
	CreateInfo.subpassCount = 1;
	CreateInfo.pSubpasses = &SubpassDescription;
	CreateInfo.dependencyCount = 1;
	CreateInfo.pDependencies = &Dependency;

	if (vkCreateRenderPass(vDevice, &CreateInfo, nullptr, &voRenderPass) != VK_SUCCESS)
	{
		spdlog::error("failed to create render pass!");
		return false;
	}
	return true;
}

bool Lemon::createFramebuffers(std::vector<VkFramebuffer>& voSwapChainFrameBuffers, const VkDevice& vDevice,
							   const std::vector<VkImageView>& vSwapChainImageViews, const VkRenderPass& vRenderPass,
							   const VkExtent2D& vSwapChainExtent)
{
	voSwapChainFrameBuffers.resize(vSwapChainImageViews.size());

	for (size_t i = 0; i < vSwapChainImageViews.size(); i++) {
		const VkImageView Attachments[] = { vSwapChainImageViews[i] };
		VkFramebufferCreateInfo CreateInfo{};
		CreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		CreateInfo.renderPass = vRenderPass;
		CreateInfo.attachmentCount = 1;
		CreateInfo.pAttachments = Attachments;
		CreateInfo.width = vSwapChainExtent.width;
		CreateInfo.height = vSwapChainExtent.height;
		CreateInfo.layers = 1;
		if (vkCreateFramebuffer(vDevice, &CreateInfo, nullptr, &voSwapChainFrameBuffers[i]) != VK_SUCCESS)
		{
			spdlog::error("failed to create framebuffer {}!", i);
			return false;
		}
	}
	return true;
}

bool Lemon::createDescriptorPool(VkDescriptorPool& voDescriptorPool, const VkDevice& vDevice, size_t vMaxFramesInFlight)
{
	VkDescriptorPoolSize PoolSize;
	PoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	PoolSize.descriptorCount = static_cast<uint32_t>(vMaxFramesInFlight);

	VkDescriptorPoolCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	CreateInfo.poolSizeCount = 1;
	CreateInfo.pPoolSizes = &PoolSize;
	CreateInfo.maxSets = static_cast<uint32_t>(vMaxFramesInFlight);

	if (vkCreateDescriptorPool(vDevice, &CreateInfo, nullptr, &voDescriptorPool) != VK_SUCCESS)
	{
		spdlog::error("failed to create descriptor pool!");
		return false;
	}
	return true;
}

bool Lemon::createDescriptorSetLayout(VkDescriptorSetLayout& voDescriptorSetLayout, const VkDevice& vDevice)
{
	VkDescriptorSetLayoutBinding UBOLayoutBinding;
	UBOLayoutBinding.binding = 0;
	UBOLayoutBinding.descriptorCount = 1;
	UBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	UBOLayoutBinding.pImmutableSamplers = nullptr;
	UBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	CreateInfo.bindingCount = 1;
	CreateInfo.pBindings = &UBOLayoutBinding;

	if (vkCreateDescriptorSetLayout(vDevice, &CreateInfo, nullptr, &voDescriptorSetLayout) != VK_SUCCESS)
	{
		spdlog::error("failed to create descriptor set layout!");
		return false;
	}
	return true;
}

bool Lemon::allocateDescriptorSets(std::vector<VkDescriptorSet>& voDescriptorSets, const VkDevice& vDevice,
	const VkDescriptorPool& vDescriptorPool, const VkDescriptorSetLayout& vDescriptorSetLayout, int vMaxFramesInFlight)
{
	const std::vector DescriptorSetLayouts(vMaxFramesInFlight, vDescriptorSetLayout);
	VkDescriptorSetAllocateInfo AllocateInfo{};
	AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	AllocateInfo.descriptorPool = vDescriptorPool;
	AllocateInfo.descriptorSetCount = static_cast<uint32_t>(vMaxFramesInFlight);
	AllocateInfo.pSetLayouts = DescriptorSetLayouts.data();

	voDescriptorSets.resize(vMaxFramesInFlight);
	if (vkAllocateDescriptorSets(vDevice, &AllocateInfo, voDescriptorSets.data()) != VK_SUCCESS)
	{
		spdlog::error("failed to allocate descriptor sets!");
		return false;
	}
	return true;
}

void Lemon::updateDescriptorSets(const VkDevice& vDevice, const std::vector<VkDescriptorSet>& vDescriptorSets,
	const std::vector<VkBuffer>& vUniformBuffers, int vMaxFramesInFlight)
{
	for (int i = 0; i < vMaxFramesInFlight; i++) {
		VkDescriptorBufferInfo BufferInfo{};
		BufferInfo.buffer = vUniformBuffers[i];
		BufferInfo.offset = 0;
		BufferInfo.range = sizeof(SUniformBufferObject);

		VkWriteDescriptorSet WriteDescriptorSet{};
		WriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSet.dstSet = vDescriptorSets[i];
		WriteDescriptorSet.dstBinding = 0;
		WriteDescriptorSet.dstArrayElement = 0;
		WriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		WriteDescriptorSet.descriptorCount = 1;
		WriteDescriptorSet.pBufferInfo = &BufferInfo;

		vkUpdateDescriptorSets(vDevice, 1, &WriteDescriptorSet, 0, nullptr);
	}
}

bool Lemon::createGraphicsPipeline(VkPipeline& voPipeline, VkPipelineLayout& voPipelineLayout,
	const VkDevice& vDevice, const VkRenderPass& vRenderPass,
	const VkDescriptorSetLayout& vDescriptorSetLayout, const std::string& vVertFilePath,
	const std::string& vFragFilePath)
{
	VkShaderModule VertShaderModule;
	VkShaderModule FragShaderModule;
	if (!createShaderModule(VertShaderModule, vVertFilePath, vDevice)) return false;
	if (!createShaderModule(FragShaderModule, vFragFilePath, vDevice)) return false;

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

	VkVertexInputBindingDescription BindingDescription;
	std::array<VkVertexInputAttributeDescription, 2> AttributeDescriptions;
	getVertexDescription(BindingDescription, AttributeDescriptions);

	VertexInputInfo.vertexBindingDescriptionCount = 1;
	VertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(AttributeDescriptions.size());
	VertexInputInfo.pVertexBindingDescriptions = &BindingDescription;
	VertexInputInfo.pVertexAttributeDescriptions = AttributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo InputAssembly{};
	InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	InputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo ViewportState{};
	ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	ViewportState.viewportCount = 1;
	ViewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo Rasterizer{};
	Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	Rasterizer.depthClampEnable = VK_FALSE;
	Rasterizer.rasterizerDiscardEnable = VK_FALSE;
	Rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	Rasterizer.lineWidth = 1.0f;
	Rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	Rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	Rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo Multisampling{};
	Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	Multisampling.sampleShadingEnable = VK_FALSE;
	Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState ColorBlendAttachment{};
	ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	ColorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo ColorBlending{};
	ColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	ColorBlending.logicOpEnable = VK_FALSE;
	ColorBlending.logicOp = VK_LOGIC_OP_COPY;
	ColorBlending.attachmentCount = 1;
	ColorBlending.pAttachments = &ColorBlendAttachment;
	ColorBlending.blendConstants[0] = 0.0f;
	ColorBlending.blendConstants[1] = 0.0f;
	ColorBlending.blendConstants[2] = 0.0f;
	ColorBlending.blendConstants[3] = 0.0f;

	std::vector<VkDynamicState> DynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo{};
	DynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	DynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(DynamicStates.size());
	DynamicStateCreateInfo.pDynamicStates = DynamicStates.data();

	VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo{};
	PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	PipelineLayoutCreateInfo.setLayoutCount = 1;
	PipelineLayoutCreateInfo.pSetLayouts = &vDescriptorSetLayout;

	if (vkCreatePipelineLayout(vDevice, &PipelineLayoutCreateInfo, nullptr, &voPipelineLayout) != VK_SUCCESS)
	{
		spdlog::error("failed to create pipeline layout!");
		return false;
	}

	VkGraphicsPipelineCreateInfo PipelineCreateInfo{};
	PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	PipelineCreateInfo.stageCount = 2;
	PipelineCreateInfo.pStages = ShaderStages;
	PipelineCreateInfo.pVertexInputState = &VertexInputInfo;
	PipelineCreateInfo.pInputAssemblyState = &InputAssembly;
	PipelineCreateInfo.pViewportState = &ViewportState;
	PipelineCreateInfo.pRasterizationState = &Rasterizer;
	PipelineCreateInfo.pMultisampleState = &Multisampling;
	PipelineCreateInfo.pColorBlendState = &ColorBlending;
	PipelineCreateInfo.pDynamicState = &DynamicStateCreateInfo;
	PipelineCreateInfo.layout = voPipelineLayout;
	PipelineCreateInfo.renderPass = vRenderPass;
	PipelineCreateInfo.subpass = 0;
	PipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(vDevice, VK_NULL_HANDLE, 1, &PipelineCreateInfo, nullptr, &voPipeline) != VK_SUCCESS)
	{
		spdlog::error("failed to create graphics pipeline!");
		return false;
	}

	vkDestroyShaderModule(vDevice, FragShaderModule, nullptr);
	vkDestroyShaderModule(vDevice, VertShaderModule, nullptr);

	return true;
}

bool Lemon::createVertexBuffer(
	VkBuffer& voVertexBuffer, VkDeviceMemory& voVertexDeviceMemory,
	const std::vector<SVertex>& vVertices,
	const VkDevice& vDevice,
	const VkPhysicalDevice& vPhysicalDevice,
	const VkCommandPool& vCommandPool,
	const VkQueue& vGraphicsQueue
)
{
	const VkDeviceSize BufferSize = sizeof(vVertices[0]) * vVertices.size();

	VkBuffer StagingBuffer;
	VkDeviceMemory StagingDeviceMemory;
	if (!createBuffer(
		StagingBuffer, StagingDeviceMemory,
		vDevice, vPhysicalDevice, BufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	)) return false;

	void* pData;
	vkMapMemory(vDevice, StagingDeviceMemory, 0, BufferSize, 0, &pData);
	memcpy(pData, vVertices.data(), BufferSize);
	vkUnmapMemory(vDevice, StagingDeviceMemory);

	if (!createBuffer(
		voVertexBuffer, voVertexDeviceMemory,
		vDevice, vPhysicalDevice, BufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	)) return false;

	copyBuffer(vDevice, vCommandPool, vGraphicsQueue, StagingBuffer, voVertexBuffer, BufferSize);

	vkDestroyBuffer(vDevice, StagingBuffer, nullptr);
	vkFreeMemory(vDevice, StagingDeviceMemory, nullptr);

	return true;
}

bool Lemon::createIndexBuffer(VkBuffer& voIndexBuffer, VkDeviceMemory& voIndexDeviceMemory,
	const std::vector<uint16_t>& vIndices, const VkDevice& vDevice, const VkPhysicalDevice& vPhysicalDevice,
	const VkCommandPool& vCommandPool, const VkQueue& vGraphicsQueue)
{
	const VkDeviceSize BufferSize = sizeof(vIndices[0]) * vIndices.size();

	VkBuffer StagingBuffer;
	VkDeviceMemory StagingDeviceMemory;
	if (!createBuffer(
		StagingBuffer, StagingDeviceMemory,
		vDevice, vPhysicalDevice, BufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	)) return false;

	void* pData;
	vkMapMemory(vDevice, StagingDeviceMemory, 0, BufferSize, 0, &pData);
	memcpy(pData, vIndices.data(), (size_t)BufferSize);
	vkUnmapMemory(vDevice, StagingDeviceMemory);

	if (!createBuffer(
		voIndexBuffer, voIndexDeviceMemory,
		vDevice, vPhysicalDevice, BufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	)) return false;

	copyBuffer(vDevice, vCommandPool, vGraphicsQueue, StagingBuffer, voIndexBuffer, BufferSize);

	vkDestroyBuffer(vDevice, StagingBuffer, nullptr);
	vkFreeMemory(vDevice, StagingDeviceMemory, nullptr);

	return true;
}

bool Lemon::createUniformBuffers(std::vector<VkBuffer>& voUniformBuffers,
	std::vector<VkDeviceMemory>& voUniformDeviceMemories, std::vector<void*>& voUniformBuffersMapped,
	const VkDevice& vDevice, const VkPhysicalDevice& vPhysicalDevice, int vMaxFramesInFlight)
{
	voUniformBuffers.resize(vMaxFramesInFlight);
	voUniformDeviceMemories.resize(vMaxFramesInFlight);
	voUniformBuffersMapped.resize(vMaxFramesInFlight);

	for (int i = 0; i < vMaxFramesInFlight; i++)
	{
		VkDeviceSize BufferSize = sizeof(SUniformBufferObject);
		if (!createBuffer(
			voUniformBuffers[i], voUniformDeviceMemories[i],
			vDevice, vPhysicalDevice, BufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		)) return false;

		vkMapMemory(vDevice, voUniformDeviceMemories[i], 0, BufferSize, 0, &voUniformBuffersMapped[i]);
	}

	return true;
}

bool Lemon::createSyncObjects(std::vector<VkSemaphore>& voImageAvailableSemaphores,
	std::vector<VkSemaphore>& voRenderFinishedSemaphores, std::vector<VkFence>& voInFlightFences,
	const VkDevice& vDevice, int vMaxFramesInFlight)
{
	voImageAvailableSemaphores.resize(vMaxFramesInFlight);
	voRenderFinishedSemaphores.resize(vMaxFramesInFlight);
	voInFlightFences.resize(vMaxFramesInFlight);

	VkSemaphoreCreateInfo SemaphoreCreateInfo{};
	SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo FenceCreateInfo{};
	FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < vMaxFramesInFlight; i++)
	{
		if (vkCreateSemaphore(vDevice, &SemaphoreCreateInfo, nullptr, &voImageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(vDevice, &SemaphoreCreateInfo, nullptr, &voRenderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(vDevice, &FenceCreateInfo, nullptr, &voInFlightFences[i]) != VK_SUCCESS)
		{
			spdlog::error("failed to create synchronization objects for a frame {}!", i);
			return false;
		}
	}
	return true;
}

void Lemon::getRequiredInstanceExtensions(std::vector<const char*>& voInstanceExtensions)
{
	uint32_t GlfwExtensionCount = 0;
	const char** GlfwExtensions = glfwGetRequiredInstanceExtensions(&GlfwExtensionCount);
	voInstanceExtensions = std::vector(GlfwExtensions, GlfwExtensions + GlfwExtensionCount);
}

void Lemon::getRequiredDeviceExtensions(std::vector<const char*>& voDeviceExtensions)
{
	voDeviceExtensions = std::vector{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
}

void Lemon::findQueueFamilyIndices(SQueueFamilyIndices& voQueueFamilyIndices, const VkPhysicalDevice& vPhysicalDevice, const VkSurfaceKHR& vSurface)
{
	uint32_t QueueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vPhysicalDevice, &QueueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(QueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(vPhysicalDevice, &QueueFamilyCount, queueFamilies.data());
	int i = 0;
	for (const auto& QueueFamily : queueFamilies)
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

void Lemon::querySwapChainSupportDetails(SSwapChainSupportDetails& voDetails, const VkPhysicalDevice& vPhysicalDevice,
	const VkSurfaceKHR& vSurface)
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

bool Lemon::isDeviceSuitable(const VkPhysicalDevice& vPhysicalDevice, const VkSurfaceKHR& vSurface, const std::vector<const char*>& vDeviceExtensions)
{
	SQueueFamilyIndices Indices;
	findQueueFamilyIndices(Indices, vPhysicalDevice, vSurface);
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
		querySwapChainSupportDetails(SwapChainSupportDetails, vPhysicalDevice, vSurface);
		IsSwapChainAdequate = !SwapChainSupportDetails._Formats.empty() && !SwapChainSupportDetails._PresentModes.empty();
	}

	return IsQueueFamiliesComplete && IsExtensionsSupported && IsSwapChainAdequate;
}

bool Lemon::readFile(std::vector<char>& voBuffer, const std::string& vFilename)
{
	std::ifstream File(vFilename, std::ios::ate | std::ios::binary);
	if (!File.is_open())
	{
		spdlog::error("failed to open file: {}!", vFilename);
		return false;
	}
	const size_t FileSize = (size_t)File.tellg();
	voBuffer.resize(FileSize);
	File.seekg(0);
	File.read(voBuffer.data(), static_cast<std::streamsize>(FileSize));
	File.close();
	return true;
}

bool Lemon::createShaderModule(VkShaderModule& voShaderModule, const std::string& vFilename, const VkDevice& vDevice)
{
	std::vector<char> Code;
	if (!readFile(Code, vFilename)) return false;

	VkShaderModuleCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	CreateInfo.codeSize = Code.size();
	CreateInfo.pCode = reinterpret_cast<const uint32_t*>(Code.data());

	if (vkCreateShaderModule(vDevice, &CreateInfo, nullptr, &voShaderModule) != VK_SUCCESS)
	{
		spdlog::error("failed to create shader module!");
		return false;
	}
	return true;
}

void Lemon::getVertexDescription(VkVertexInputBindingDescription& voBindingDescription,
	std::array<VkVertexInputAttributeDescription, 2>& voAttributeDescriptions)
{
	voBindingDescription.binding = 0;
	voBindingDescription.stride = sizeof(SVertex);
	voBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	voAttributeDescriptions[0].binding = 0;
	voAttributeDescriptions[0].location = 0;
	voAttributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	voAttributeDescriptions[0].offset = offsetof(SVertex, _Pos);

	voAttributeDescriptions[1].binding = 0;
	voAttributeDescriptions[1].location = 1;
	voAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	voAttributeDescriptions[1].offset = offsetof(SVertex, _Color);
}

bool Lemon::findMemoryType(uint32_t& voType, uint32_t vTypeFilter, const VkPhysicalDevice& vPhysicalDevice,
	const VkMemoryPropertyFlags& vPropertyFlags)
{
	VkPhysicalDeviceMemoryProperties MemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(vPhysicalDevice, &MemoryProperties);
	for (uint32_t i = 0; i < MemoryProperties.memoryTypeCount; i++)
	{
		if ((vTypeFilter & (1 << i)) && (MemoryProperties.memoryTypes[i].propertyFlags & vPropertyFlags) == vPropertyFlags)
		{
			voType = i;
			return true;
		}
	}
	spdlog::error("failed to find suitable memory type!");
	return false;
}

bool Lemon::createBuffer(VkBuffer& voBuffer, VkDeviceMemory& voDeviceMemory, const VkDevice& vDevice,
	const VkPhysicalDevice& vPhysicalDevice, const VkDeviceSize& vBufferSize, const VkBufferUsageFlags& vUsageFlags,
	const VkMemoryPropertyFlags& vPropertyFlags)
{
	VkBufferCreateInfo BufferCreateInfo{};
	BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	BufferCreateInfo.size = vBufferSize;
	BufferCreateInfo.usage = vUsageFlags;
	BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(vDevice, &BufferCreateInfo, nullptr, &voBuffer) != VK_SUCCESS)
	{
		spdlog::error("failed to create voBuffer!");
		return false;
	}

	VkMemoryRequirements MemoryRequirements;
	vkGetBufferMemoryRequirements(vDevice, voBuffer, &MemoryRequirements);

	VkMemoryAllocateInfo AllocateInfo{};
	AllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	AllocateInfo.allocationSize = MemoryRequirements.size;
	findMemoryType(AllocateInfo.memoryTypeIndex, MemoryRequirements.memoryTypeBits, vPhysicalDevice, vPropertyFlags);

	if (vkAllocateMemory(vDevice, &AllocateInfo, nullptr, &voDeviceMemory) != VK_SUCCESS)
	{
		spdlog::error("failed to allocate voBuffer memory!");
		return false;
	}

	vkBindBufferMemory(vDevice, voBuffer, voDeviceMemory, 0);

	return true;
}

void Lemon::copyBuffer(const VkDevice& vDevice, const VkCommandPool& vCommandPool, const VkQueue& vGraphicsQueue,
	const VkBuffer& vSrcBuffer, const VkBuffer& vDstBuffer, const VkDeviceSize& vBufferSize)
{
	VkCommandBufferAllocateInfo AllocateInfo{};
	AllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AllocateInfo.commandPool = vCommandPool;
	AllocateInfo.commandBufferCount = 1;

	VkCommandBuffer CommandBuffer;
	vkAllocateCommandBuffers(vDevice, &AllocateInfo, &CommandBuffer);

	VkCommandBufferBeginInfo BeginInfo{};
	BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(CommandBuffer, &BeginInfo);

	VkBufferCopy CopyRegion{};
	CopyRegion.size = vBufferSize;
	vkCmdCopyBuffer(CommandBuffer, vSrcBuffer, vDstBuffer, 1, &CopyRegion);

	vkEndCommandBuffer(CommandBuffer);

	VkSubmitInfo SubmitInfo{};
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &CommandBuffer;

	vkQueueSubmit(vGraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vGraphicsQueue);

	vkFreeCommandBuffers(vDevice, vCommandPool, 1, &CommandBuffer);
}
