#include "pch.h"
#include "SwapChain.h"
#include "Device.h"

bool Lemon::CSwapChain::init(const CDevice* vDevice, VkExtent2D vExtent)
{
	if (vDevice == nullptr) return false;
	m_pDevice = vDevice;
	if (!__createSwapChain(vExtent)) return false;
	__getSwapChainImages();
	if (!__createImageViews()) return false;
	if (!__createRenderPass()) return false;
	if (!__createSwapChainFramebuffers()) return false;
	if (!__createSyncObjects()) return false;
	return true;
}

void Lemon::CSwapChain::cleanup()
{
	__cleanupSwapChain();
	__cleanupSyncObjects();
	m_pDevice = nullptr;
}

bool Lemon::CSwapChain::recreate(VkExtent2D vExtent)
{
	vkDeviceWaitIdle(m_pDevice->getDevice());
	__cleanupSwapChain();
	if (!__createSwapChain(vExtent)) return false;
	__getSwapChainImages();
	if (!__createImageViews()) return false;
	if (!__createRenderPass()) return false;
	if (!__createSwapChainFramebuffers()) return false;
	return true;
}

VkResult Lemon::CSwapChain::acquireNextImage(uint32_t* voImageIndex) const
{
	vkWaitForFences(m_pDevice->getDevice(), 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);
	vkResetFences(m_pDevice->getDevice(), 1, &m_InFlightFences[m_CurrentFrame]);
	const VkResult Result = vkAcquireNextImageKHR(m_pDevice->getDevice(), m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, voImageIndex);
	return Result;
}

VkResult Lemon::CSwapChain::submitCommandBuffers(VkCommandBuffer vCommandBuffer, uint32_t vImageIndex)
{
	const VkSemaphore WaitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
	const VkSemaphore SignalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
	constexpr VkPipelineStageFlags WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo SubmitInfo{};
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.waitSemaphoreCount = 1;
	SubmitInfo.pWaitSemaphores = WaitSemaphores;
	SubmitInfo.pWaitDstStageMask = WaitStages;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &vCommandBuffer;
	SubmitInfo.signalSemaphoreCount = 1;
	SubmitInfo.pSignalSemaphores = SignalSemaphores;
	if (vkQueueSubmit(m_pDevice->getGraphicsQueue(), 1, &SubmitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR PresentInfo{};
	PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	PresentInfo.waitSemaphoreCount = 1;
	PresentInfo.pWaitSemaphores = SignalSemaphores;
	PresentInfo.swapchainCount = 1;
	PresentInfo.pSwapchains = &m_SwapChain;
	PresentInfo.pImageIndices = &vImageIndex;
	const VkResult Result = vkQueuePresentKHR(m_pDevice->getPresentQueue(), &PresentInfo);

	m_CurrentFrame = (m_CurrentFrame + 1) % m_MaxFramesInFlight;
	return Result;
}

bool Lemon::CSwapChain::__createSwapChain(VkExtent2D vExtent)
{
	SSwapChainSupportDetails Details;
	m_pDevice->querySwapChainSupportDetails(Details);
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
		m_SwapChainExtent = Details._Capabilities.currentExtent;
	}
	else
	{
		m_SwapChainExtent.width = std::clamp(vExtent.width, Details._Capabilities.minImageExtent.width, Details._Capabilities.maxImageExtent.width);
		m_SwapChainExtent.height = std::clamp(vExtent.height, Details._Capabilities.minImageExtent.height, Details._Capabilities.maxImageExtent.height);
	}

	uint32_t ImageCount = Details._Capabilities.minImageCount + 1;
	if (Details._Capabilities.maxImageCount > 0 && ImageCount > Details._Capabilities.maxImageCount) {
		ImageCount = Details._Capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	CreateInfo.surface = m_pDevice->getSurface();
	CreateInfo.minImageCount = ImageCount;
	CreateInfo.imageFormat = SurfaceFormat.format;
	CreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
	CreateInfo.imageExtent = m_SwapChainExtent;
	CreateInfo.imageArrayLayers = 1;
	CreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	const auto& Indices = m_pDevice->getQueueFamilyIndices();
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

	if (vkCreateSwapchainKHR(m_pDevice->getDevice(), &CreateInfo, nullptr, &m_SwapChain) != VK_SUCCESS)
	{
		spdlog::error("failed to create swap chain!");
		return false;
	}

	m_SwapChainImageFormat = SurfaceFormat.format;
	spdlog::info("created swap chain: {} {}", m_SwapChainExtent.width, m_SwapChainExtent.height);
	return true;
}

void Lemon::CSwapChain::__getSwapChainImages()
{
	uint32_t ImageCount;
	vkGetSwapchainImagesKHR(m_pDevice->getDevice(), m_SwapChain, &ImageCount, nullptr);
	m_SwapChainImages.resize(ImageCount);
	vkGetSwapchainImagesKHR(m_pDevice->getDevice(), m_SwapChain, &ImageCount, m_SwapChainImages.data());
}

bool Lemon::CSwapChain::__createImageViews()
{
	m_SwapChainImageViews.resize(m_SwapChainImages.size());
	for (size_t i = 0; i < m_SwapChainImages.size(); i++)
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
		if (vkCreateImageView(m_pDevice->getDevice(), &CreateInfo, nullptr, &m_SwapChainImageViews[i]) != VK_SUCCESS)
		{
			spdlog::error("failed to create image views {}!", i);
			return false;
		}
	}
	spdlog::info("created swap chain image views");
	return true;
}

bool Lemon::CSwapChain::__createSwapChainFramebuffers()
{
	m_SwapChainFramebuffers.resize(m_SwapChainImageViews.size());

	for (size_t i = 0; i < m_SwapChainImageViews.size(); i++) {
		const VkImageView Attachments[] = { m_SwapChainImageViews[i] };
		VkFramebufferCreateInfo CreateInfo{};
		CreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		CreateInfo.renderPass = m_RenderPass;
		CreateInfo.attachmentCount = 1;
		CreateInfo.pAttachments = Attachments;
		CreateInfo.width = m_SwapChainExtent.width;
		CreateInfo.height = m_SwapChainExtent.height;
		CreateInfo.layers = 1;
		if (vkCreateFramebuffer(m_pDevice->getDevice(), &CreateInfo, nullptr, &m_SwapChainFramebuffers[i]) != VK_SUCCESS)
		{
			spdlog::error("failed to create framebuffer {}!", i);
			return false;
		}
	}
	spdlog::info("created frame buffers");
	return true;
}

void Lemon::CSwapChain::__cleanupSwapChain()
{
	for (const auto Framebuffer : m_SwapChainFramebuffers)
	{
		vkDestroyFramebuffer(m_pDevice->getDevice(), Framebuffer, nullptr);
	}
	m_SwapChainFramebuffers.clear();
	vkDestroyRenderPass(m_pDevice->getDevice(), m_RenderPass, nullptr);
	m_RenderPass = VK_NULL_HANDLE;
	for (const auto ImageView : m_SwapChainImageViews)
	{
		vkDestroyImageView(m_pDevice->getDevice(), ImageView, nullptr);
	}
	m_SwapChainImageViews.clear();
	m_SwapChainImages.clear();
	vkDestroySwapchainKHR(m_pDevice->getDevice(), m_SwapChain, nullptr);
	m_SwapChain = VK_NULL_HANDLE;
}

bool Lemon::CSwapChain::__createSyncObjects()
{
	m_ImageAvailableSemaphores.resize(m_MaxFramesInFlight);
	m_RenderFinishedSemaphores.resize(m_MaxFramesInFlight);
	m_InFlightFences.resize(m_MaxFramesInFlight);

	VkSemaphoreCreateInfo SemaphoreCreateInfo{};
	SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo FenceCreateInfo{};
	FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < m_MaxFramesInFlight; i++)
	{
		if (vkCreateSemaphore(m_pDevice->getDevice(), &SemaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_pDevice->getDevice(), &SemaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(m_pDevice->getDevice(), &FenceCreateInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS)
		{
			spdlog::error("failed to create synchronization objects for a frame {}!", i);
			return false;
		}
	}
	spdlog::info("created sync objects");
	return true;
}

void Lemon::CSwapChain::__cleanupSyncObjects()
{
	for (size_t i = 0; i < m_InFlightFences.size(); i++)
	{
		vkDestroySemaphore(m_pDevice->getDevice(), m_RenderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_pDevice->getDevice(), m_ImageAvailableSemaphores[i], nullptr);
		vkDestroyFence(m_pDevice->getDevice(), m_InFlightFences[i], nullptr);
	}
	m_RenderFinishedSemaphores.clear();
	m_ImageAvailableSemaphores.clear();
	m_InFlightFences.clear();
}

bool Lemon::CSwapChain::__createRenderPass()
{
	VkAttachmentDescription ColorAttachment{};
	ColorAttachment.format = m_SwapChainImageFormat;
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

	if (vkCreateRenderPass(m_pDevice->getDevice(), &CreateInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
	{
		spdlog::error("failed to create render pass!");
		return false;
	}
	spdlog::info("created render pass");
	return true;
}
