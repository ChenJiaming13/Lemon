#include "HelloTriangleApplication.h"

void CHelloTriangleApplication::__init()
{
	constexpr int Width = 800;
	constexpr int Height = 600;
	m_Window.init(Width, Height, "Vulkan Triangle");
	m_Window.addFrameBufferSizeCallback([this](int, int) {this->m_IsFramebufferResized = true; });
	m_Device.init(&m_Window);
	m_SwapChain.init(&m_Device, { Width, Height });
	Lemon::SRenderPipelineCreateInfo CreateInfo;
	CreateInfo._VertFilePath = "../Assets/Shaders/vert1.spv";
	CreateInfo._FragFilePath = "../Assets/Shaders/frag1.spv";
	m_RenderPipeline.init(&m_Device, &m_SwapChain, CreateInfo);
	const std::vector<Lemon::CMesh::SVertex> Vertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};
	const std::vector<uint16_t> Indices = {
		0, 1, 2, 2, 3, 0
	};
	m_Mesh.init(&m_Device, Vertices, Indices);

	m_CommandBuffers.resize(m_SwapChain.getMaxFramesInFlight());
	m_Device.allocatePrimaryCommandBuffer(m_SwapChain.getMaxFramesInFlight(), m_CommandBuffers.data());
}

void CHelloTriangleApplication::__mainLoop()
{
	while (!m_Window.shouldCloseWindow())
	{
		Lemon::CGlfwWindow::pollEvents();
		__drawFrame();
	}
	vkDeviceWaitIdle(m_Device.getDevice());
}

void CHelloTriangleApplication::__cleanup()
{
	m_Mesh.cleanup();
	m_SwapChain.cleanup();
	m_RenderPipeline.cleanup();
	m_Device.cleanup();
	m_Window.cleanup();
}

void CHelloTriangleApplication::__drawFrame()
{
	const uint32_t CurrentFrame = m_SwapChain.getCurrentFrame();
	uint32_t ImageIndex;
	m_SwapChain.acquireNextImage(&ImageIndex);
	vkResetCommandBuffer(m_CommandBuffers[CurrentFrame], 0);
	__recordCommandBuffer(m_CommandBuffers[CurrentFrame], ImageIndex);
	if (const VkResult Result = m_SwapChain.submitCommandBuffers(m_CommandBuffers[CurrentFrame], ImageIndex);
		Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR || m_IsFramebufferResized)
	{
		__recreateSwapChain();
		m_IsFramebufferResized = false;
	}
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
	RenderPassBeginInfo.renderPass = m_SwapChain.getRenderPass();
	RenderPassBeginInfo.framebuffer = m_SwapChain.getFrameBuffer(vImageIndex);
	RenderPassBeginInfo.renderArea.offset = { 0, 0 };
	RenderPassBeginInfo.renderArea.extent = m_SwapChain.getSwapChainExtent();
	constexpr VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
	RenderPassBeginInfo.clearValueCount = 1;
	RenderPassBeginInfo.pClearValues = &clearColor;
	vkCmdBeginRenderPass(vCommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(vCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_RenderPipeline.getGraphicsPipeline());

	VkViewport Viewport;
	Viewport.x = 0.0f;
	Viewport.y = 0.0f;
	Viewport.width = static_cast<float>(m_SwapChain.getSwapChainExtent().width);
	Viewport.height = static_cast<float>(m_SwapChain.getSwapChainExtent().height);
	Viewport.minDepth = 0.0f;
	Viewport.maxDepth = 1.0f;
	vkCmdSetViewport(vCommandBuffer, 0, 1, &Viewport);

	VkRect2D Scissor;
	Scissor.offset = { 0, 0 };
	Scissor.extent = m_SwapChain.getSwapChainExtent();
	vkCmdSetScissor(vCommandBuffer, 0, 1, &Scissor);

	m_Mesh.draw(vCommandBuffer);

	vkCmdEndRenderPass(vCommandBuffer);

	if (vkEndCommandBuffer(vCommandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}


void CHelloTriangleApplication::__recreateSwapChain()
{
	int Width, Height;
	m_Window.getFrameBufferSize(&Width, &Height);
	m_SwapChain.recreate({ static_cast<uint32_t>(Width), static_cast<uint32_t>(Height) });
}
