#include "HelloTriangleApplication.h"

void CHelloTriangleApplication::__init()
{
	constexpr int Width = 800;
	constexpr int Height = 600;
	m_pWindow = Lemon::createWindow(Width, Height, "Vulkan Triangle");
	glfwSetWindowUserPointer(m_pWindow, this);
	glfwSetFramebufferSizeCallback(m_pWindow, [](GLFWwindow* vWindow, int vWidth, int vHeight) {
		const auto pApp = static_cast<CHelloTriangleApplication*>(glfwGetWindowUserPointer(vWindow));
		pApp->m_IsFramebufferResized = true;
		});

	std::vector<const char*> InstanceExtensions;
	Lemon::getRequiredInstanceExtensions(InstanceExtensions);
	_ASSERTE(Lemon::createInstance(m_Instance, InstanceExtensions));
	_ASSERTE(Lemon::createSurface(m_Surface, m_Instance, m_pWindow));
	std::vector<const char*> DeviceExtensions;
	Lemon::getRequiredDeviceExtensions(DeviceExtensions);
	_ASSERTE(Lemon::pickPhysicalDevice(m_PhysicalDevice, m_Instance, m_Surface, DeviceExtensions));
	_ASSERTE(Lemon::createLogicalDeviceAndQueues(m_Device, m_GraphicsQueue, m_PresentQueue, m_PhysicalDevice, m_Surface, DeviceExtensions));
	_ASSERTE(Lemon::createSwapChain(m_SwapChain, m_SwapChainImageFormat, m_SwapChainExtent, m_Device, m_PhysicalDevice, m_Surface, Width, Height));
	Lemon::getSwapChainImages(m_SwapChainImages, m_Device, m_SwapChain);
	_ASSERTE(Lemon::createImageViews(m_SwapChainImageViews, m_Device, m_SwapChainImages, m_SwapChainImageFormat));
	_ASSERTE(Lemon::createRenderPass(m_RenderPass, m_Device, m_SwapChainImageFormat));
	_ASSERTE(Lemon::createDescriptorSetLayout(m_DescriptorSetLayout, m_Device));
	_ASSERTE(Lemon::createGraphicsPipeline(m_GraphicsPipeline, m_PipelineLayout, m_Device, m_RenderPass, m_DescriptorSetLayout,
		"C:/Users/Chen/Documents/Code/Lemon/assets/shaders/vert2.spv",
		"C:/Users/Chen/Documents/Code/Lemon/assets/shaders/frag2.spv"
	));
	_ASSERTE(Lemon::createFramebuffers(m_SwapChainFramebuffers, m_Device, m_SwapChainImageViews, m_RenderPass, m_SwapChainExtent));
	_ASSERTE(Lemon::createCommandPool(m_CommandPool, m_Device, m_PhysicalDevice, m_Surface));
	_ASSERTE(Lemon::createVertexBuffer(m_VertexBuffer, m_VertexDeviceMemory, VERTICES, m_Device, m_PhysicalDevice, m_CommandPool, m_GraphicsQueue));
	_ASSERTE(Lemon::createIndexBuffer(m_IndexBuffer, m_IndexDeviceMemory, INDICES, m_Device, m_PhysicalDevice, m_CommandPool, m_GraphicsQueue));
	_ASSERTE(Lemon::createUniformBuffers(m_UniformBuffers, m_UniformDevicesMemory, m_UniformBuffersMapped, m_Device, m_PhysicalDevice, MAX_FRAMES_IN_FLIGHT));
	_ASSERTE(Lemon::createDescriptorPool(m_DescriptorPool, m_Device, MAX_FRAMES_IN_FLIGHT));
	_ASSERTE(Lemon::allocateDescriptorSets(m_DescriptorSets, m_Device, m_DescriptorPool, m_DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT));
	Lemon::updateDescriptorSets(m_Device, m_DescriptorSets, m_UniformBuffers, MAX_FRAMES_IN_FLIGHT);
	_ASSERTE(Lemon::allocateCommandBuffers(m_CommandBuffers, m_Device, m_CommandPool, MAX_FRAMES_IN_FLIGHT));
	_ASSERTE(Lemon::createSyncObjects(m_ImageAvailableSemaphores, m_RenderFinishedSemaphores, m_InFlightFences, m_Device, MAX_FRAMES_IN_FLIGHT));
}

void CHelloTriangleApplication::__mainLoop()
{
	while (!glfwWindowShouldClose(m_pWindow))
	{
		glfwPollEvents();
		__drawFrame();
	}
	vkDeviceWaitIdle(m_Device);
}

void CHelloTriangleApplication::__cleanupSwapChain() const
{
	for (const auto Framebuffer : m_SwapChainFramebuffers) {
		vkDestroyFramebuffer(m_Device, Framebuffer, nullptr);
	}
	for (const auto ImageView : m_SwapChainImageViews) {
		vkDestroyImageView(m_Device, ImageView, nullptr);
	}
	vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
}

void CHelloTriangleApplication::__cleanup() const
{
	__cleanupSwapChain();

	vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
	vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroyBuffer(m_Device, m_UniformBuffers[i], nullptr);
		vkFreeMemory(m_Device, m_UniformDevicesMemory[i], nullptr);
	}

	vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);

	vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);

	vkDestroyBuffer(m_Device, m_IndexBuffer, nullptr);
	vkFreeMemory(m_Device, m_IndexDeviceMemory, nullptr);

	vkDestroyBuffer(m_Device, m_VertexBuffer, nullptr);
	vkFreeMemory(m_Device, m_VertexDeviceMemory, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
		vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

	vkDestroyDevice(m_Device, nullptr);


	vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
	vkDestroyInstance(m_Instance, nullptr);

	glfwDestroyWindow(m_pWindow);

	glfwTerminate();
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
	_ASSERTE(Lemon::createSwapChain(m_SwapChain, m_SwapChainImageFormat, m_SwapChainExtent, m_Device, m_PhysicalDevice, m_Surface, Width, Height));
	Lemon::getSwapChainImages(m_SwapChainImages, m_Device, m_SwapChain);
	_ASSERTE(Lemon::createImageViews(m_SwapChainImageViews, m_Device, m_SwapChainImages, m_SwapChainImageFormat));
	_ASSERTE(Lemon::createFramebuffers(m_SwapChainFramebuffers, m_Device, m_SwapChainImageViews, m_RenderPass, m_SwapChainExtent));
}

void CHelloTriangleApplication::__recordCommandBuffer(VkCommandBuffer vCommandBuffer, uint32_t vImageIndex) const
{
	VkCommandBufferBeginInfo BeginInfo{};
	BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(vCommandBuffer, &BeginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo RenderPassBeginInfo{};
	RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	RenderPassBeginInfo.renderPass = m_RenderPass;
	RenderPassBeginInfo.framebuffer = m_SwapChainFramebuffers[vImageIndex];
	RenderPassBeginInfo.renderArea.offset = { 0, 0 };
	RenderPassBeginInfo.renderArea.extent = m_SwapChainExtent;

	constexpr VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
	RenderPassBeginInfo.clearValueCount = 1;
	RenderPassBeginInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(vCommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(vCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

	VkViewport Viewport;
	Viewport.x = 0.0f;
	Viewport.y = 0.0f;
	Viewport.width = static_cast<float>(m_SwapChainExtent.width);
	Viewport.height = static_cast<float>(m_SwapChainExtent.height);
	Viewport.minDepth = 0.0f;
	Viewport.maxDepth = 1.0f;
	vkCmdSetViewport(vCommandBuffer, 0, 1, &Viewport);

	VkRect2D Scissor;
	Scissor.offset = { 0, 0 };
	Scissor.extent = m_SwapChainExtent;
	vkCmdSetScissor(vCommandBuffer, 0, 1, &Scissor);

	const VkBuffer vertexBuffers[] = { m_VertexBuffer };
	constexpr VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(vCommandBuffer, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(vCommandBuffer, m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);

	vkCmdBindDescriptorSets(vCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSets[m_CurrentFrame], 0, nullptr);

	vkCmdDrawIndexed(vCommandBuffer, static_cast<uint32_t>(INDICES.size()), 1, 0, 0, 0);

	vkCmdEndRenderPass(vCommandBuffer);

	if (vkEndCommandBuffer(vCommandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}

void CHelloTriangleApplication::__updateUniformBuffer(uint32_t vCurrentImage) const
{
	static auto StartTime = std::chrono::high_resolution_clock::now();
	const auto CurrentTime = std::chrono::high_resolution_clock::now();
	const float Time = std::chrono::duration<float>(CurrentTime - StartTime).count();

	Lemon::SUniformBufferObject UBO;
	UBO._Model = glm::rotate(glm::mat4(1.0f), Time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	UBO._View = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	UBO._Proj = glm::perspective(glm::radians(45.0f), static_cast<float>(m_SwapChainExtent.width) / static_cast<float>(m_SwapChainExtent.height), 0.1f, 10.0f);
	UBO._Proj[1][1] *= -1;

	memcpy(m_UniformBuffersMapped[vCurrentImage], &UBO, sizeof(UBO));
}

void CHelloTriangleApplication::__drawFrame()
{
	vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		__recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	__updateUniformBuffer(m_CurrentFrame);

	vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);

	vkResetCommandBuffer(m_CommandBuffers[m_CurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
	__recordCommandBuffer(m_CommandBuffers[m_CurrentFrame], imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_CommandBuffers[m_CurrentFrame];

	VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { m_SwapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_IsFramebufferResized) {
		m_IsFramebufferResized = false;
		__recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
