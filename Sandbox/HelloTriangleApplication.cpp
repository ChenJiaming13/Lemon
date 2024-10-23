#include "HelloTriangleApplication.h"
#include <spdlog/spdlog.h>
#include <glm/gtc/matrix_transform.hpp>
#include "Buffer.h"

void CHelloTriangleApplication::__init()
{
	constexpr int Width = 800;
	constexpr int Height = 600;
	m_Window.init(Width, Height, "Vulkan Triangle");
	m_Window.addFrameBufferSizeCallback([this](int, int) {this->m_IsFramebufferResized = true; });
	m_Device.init(&m_Window);
	m_SwapChain.init(&m_Device, { Width, Height });
	__createPipelineLayout();
	Lemon::SRenderPipelineCreateInfo CreateInfo;
	CreateInfo._VertFilePath = "../Assets/Shaders/spv/shader4.vert.spv";
	CreateInfo._FragFilePath = "../Assets/Shaders/spv/shader4.frag.spv";
	CreateInfo._PipelineLayout = m_PipelineLayout;
	m_RenderPipeline.init(&m_Device, &m_SwapChain, CreateInfo);
	const std::vector<Lemon::CMesh::SVertex> Vertices = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
	};
	const std::vector<uint16_t> Indices = {
		0, 1, 2, 2, 3, 0
	};
	m_Mesh.init(&m_Device, Vertices, Indices);

	m_CommandBuffers.resize(m_SwapChain.getMaxFramesInFlight());
	m_Device.allocatePrimaryCommandBuffer(m_SwapChain.getMaxFramesInFlight(), m_CommandBuffers.data());

	__createUniformBuffers();
	m_DescriptorPool.init(&m_Device, m_SwapChain.getMaxFramesInFlight(), { 
		{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = m_SwapChain.getMaxFramesInFlight()},
		{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = m_SwapChain.getMaxFramesInFlight()},
	});
	m_MainTexture.init(&m_Device, "../Assets/Textures/texture.jpg");
	__createDescriptorSets();
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
	for (const auto pUniformBuffer : m_UniformBuffers) delete pUniformBuffer;
	m_UniformBuffers.clear();
	m_DescriptorPool.cleanup();
	m_DescriptorSets.clear();
	vkDestroyDescriptorSetLayout(m_Device.getDevice(), m_DescriptorSetLayout, nullptr);
	m_DescriptorSetLayout = VK_NULL_HANDLE;
	vkDestroyPipelineLayout(m_Device.getDevice(), m_PipelineLayout, nullptr);
	m_PipelineLayout = VK_NULL_HANDLE;
	m_Mesh.cleanup();
	m_MainTexture.cleanup();
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
	__recordCommandBuffer(m_CommandBuffers[CurrentFrame], ImageIndex, CurrentFrame);
	if (const VkResult Result = m_SwapChain.submitCommandBuffers(m_CommandBuffers[CurrentFrame], ImageIndex);
		Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR || m_IsFramebufferResized)
	{
		__recreateSwapChain();
		m_IsFramebufferResized = false;
	}
}

void CHelloTriangleApplication::__recordCommandBuffer(VkCommandBuffer vCommandBuffer, uint32_t vImageIndex, uint32_t vCurrentFrame) const
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

	static auto StartTime = std::chrono::high_resolution_clock::now();
	const auto CurrentTime = std::chrono::high_resolution_clock::now();
	const float Time = std::chrono::duration<float>(CurrentTime - StartTime).count();

	SModelViewProj ModelViewProj;
	ModelViewProj._Model = glm::rotate(glm::mat4(1.0f), Time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ModelViewProj._View = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ModelViewProj._Proj = glm::perspective(
		glm::radians(45.0f),
		static_cast<float>(m_SwapChain.getSwapChainExtent().width) / static_cast<float>(m_SwapChain.getSwapChainExtent().height),
		0.1f, 10.0f
	);
	ModelViewProj._Proj[1][1] *= -1;
	//vkCmdPushConstants(
	//	vCommandBuffer,
	//	m_PipelineLayout,
	//	VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	//	0,
	//	sizeof(SModelViewProj),
	//	&ModelViewProj
	//);
	memcpy(m_UniformBuffers[vCurrentFrame]->getMappedPtr(), &ModelViewProj, sizeof(SModelViewProj));

	vkCmdBindDescriptorSets(
		vCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
		0, 1, &m_DescriptorSets[vCurrentFrame], 0, nullptr
	);

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

bool CHelloTriangleApplication::__createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding UBOLayoutBinding;
	UBOLayoutBinding.binding = 0;
	UBOLayoutBinding.descriptorCount = 1;
	UBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	UBOLayoutBinding.pImmutableSamplers = nullptr;
	UBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding SamplerLayoutBinding;
	SamplerLayoutBinding.binding = 1;
	SamplerLayoutBinding.descriptorCount = 1;
	SamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	SamplerLayoutBinding.pImmutableSamplers = nullptr;
	SamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	const std::array SetLayoutBindings = {
		UBOLayoutBinding, SamplerLayoutBinding
	};

	VkDescriptorSetLayoutCreateInfo SetLayoutCreateInfo{};
	SetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	SetLayoutCreateInfo.bindingCount = SetLayoutBindings.size();
	SetLayoutCreateInfo.pBindings = SetLayoutBindings.data();
	if (vkCreateDescriptorSetLayout(m_Device.getDevice(), &SetLayoutCreateInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS)
	{
		spdlog::error("failed to create descriptor set layout!");
		return false;
	}
	spdlog::info("created descriptor set layout");
	return true;
}

bool CHelloTriangleApplication::__createPipelineLayout()
{
	//VkPushConstantRange PushConstantRange;
	//PushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	//PushConstantRange.offset = 0;
	//PushConstantRange.size = sizeof(SModelViewProj);

	if (!__createDescriptorSetLayout()) return false;

	VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo{};
	PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	PipelineLayoutCreateInfo.setLayoutCount = 1;
	PipelineLayoutCreateInfo.pSetLayouts = &m_DescriptorSetLayout;
	PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	//PipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	//PipelineLayoutCreateInfo.pPushConstantRanges = &PushConstantRange;

	if (vkCreatePipelineLayout(m_Device.getDevice(), &PipelineLayoutCreateInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
	{
		spdlog::error("failed to create pipeline layout!");
		return false;
	}
	spdlog::info("created pipeline layout");
	return true;
}

bool CHelloTriangleApplication::__createDescriptorSets()
{
	const auto MaxFramesInFlight = m_SwapChain.getMaxFramesInFlight();
	if (const std::vector SetLayouts(MaxFramesInFlight, m_DescriptorSetLayout); !m_DescriptorPool.allocateDescriptorSets(SetLayouts, m_DescriptorSets)) return false;

	for (size_t i = 0; i < MaxFramesInFlight; i++)
	{
		VkDescriptorBufferInfo BufferInfo{};
		BufferInfo.buffer = m_UniformBuffers[i]->getBuffer();
		BufferInfo.offset = 0;
		BufferInfo.range = m_UniformBuffers[i]->getBufferSize();

		VkDescriptorImageInfo ImageInfo;
		m_MainTexture.getDescriptorImageInfo(&ImageInfo);

		std::vector<VkWriteDescriptorSet> WriteDescriptorSets;
		WriteDescriptorSets.resize(2);
		WriteDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[0].dstSet = m_DescriptorSets[i];
		WriteDescriptorSets[0].dstBinding = 0;
		WriteDescriptorSets[0].dstArrayElement = 0;
		WriteDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		WriteDescriptorSets[0].descriptorCount = 1;
		WriteDescriptorSets[0].pBufferInfo = &BufferInfo;

		WriteDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[1].dstSet = m_DescriptorSets[i];
		WriteDescriptorSets[1].dstBinding = 1;
		WriteDescriptorSets[1].dstArrayElement = 0;
		WriteDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		WriteDescriptorSets[1].descriptorCount = 1;
		WriteDescriptorSets[1].pImageInfo = &ImageInfo;

		vkUpdateDescriptorSets(m_Device.getDevice(), WriteDescriptorSets.size(), WriteDescriptorSets.data(), 0, nullptr);
	}
	spdlog::info("bound descriptor sets");

	return true;
}

bool CHelloTriangleApplication::__createUniformBuffers()
{
	m_UniformBuffers.resize(m_SwapChain.getMaxFramesInFlight());
	for (auto& pUniformBuffer : m_UniformBuffers)
	{
		constexpr VkDeviceSize BufferSize = sizeof(SModelViewProj);
		const auto pBuffer = new Lemon::CBuffer;
		pBuffer->init(&m_Device, BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		pBuffer->mapMemory(0, BufferSize);
		pUniformBuffer = pBuffer;
	}
	spdlog::info("created uniform buffers");
	return true;
}
