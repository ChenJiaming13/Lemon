#include "pch.h"
#include "RenderPipeline.h"
#include "Device.h"
#include "Mesh.h"
#include "SwapChain.h"

bool Lemon::CRenderPipeline::init(const CDevice* vDevice, const CSwapChain* vSwapChain, const SRenderPipelineCreateInfo& vCreateInfo)
{
	m_pDevice = vDevice;
	m_pSwapChain = vSwapChain;
	//if (!__createDescriptorSetLayout()) return false;
	if (!__createGraphicsPipeline(vCreateInfo)) return false;
	return true;
}

void Lemon::CRenderPipeline::cleanup()
{
	if (m_GraphicsPipeline == nullptr) return;
	vkDestroyPipeline(m_pDevice->getDevice(), m_GraphicsPipeline, nullptr);
	vkDestroyDescriptorSetLayout(m_pDevice->getDevice(), m_DescriptorSetLayout, nullptr);
	m_GraphicsPipeline = VK_NULL_HANDLE;
	m_DescriptorSetLayout = VK_NULL_HANDLE;
	m_pDevice = nullptr;
	m_pSwapChain = nullptr;
}

bool Lemon::CRenderPipeline::__createDescriptorSetLayout()
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

	if (vkCreateDescriptorSetLayout(m_pDevice->getDevice(), &CreateInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS)
	{
		spdlog::error("failed to create descriptor set layout!");
		return false;
	}
	return true;
}

bool Lemon::CRenderPipeline::__createShaderModule(const std::string& vFilename, VkShaderModule* voShaderModule) const
{
	std::vector<char> Code;
	if (!__readFile(vFilename, Code)) return false;

	VkShaderModuleCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	CreateInfo.codeSize = Code.size();
	CreateInfo.pCode = reinterpret_cast<const uint32_t*>(Code.data());

	if (vkCreateShaderModule(m_pDevice->getDevice(), &CreateInfo, nullptr, voShaderModule) != VK_SUCCESS)
	{
		spdlog::error("failed to create shader module!");
		return false;
	}
	spdlog::info("created shader module: {}", vFilename);
	return true;
}

bool Lemon::CRenderPipeline::__createGraphicsPipeline(const SRenderPipelineCreateInfo& vCreateInfo)
{
	VkShaderModule VertShaderModule;
	VkShaderModule FragShaderModule;
	if (!__createShaderModule(vCreateInfo._VertFilePath, &VertShaderModule)) return false;
	if (!__createShaderModule(vCreateInfo._FragFilePath, &FragShaderModule)) return false;

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
	CMesh::SVertex::getVertexDescription(BindingDescription, AttributeDescriptions);

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
	//Rasterizer.cullMode = VK_CULL_MODE_NONE;
	//Rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
	PipelineCreateInfo.layout = vCreateInfo._PipelineLayout;
	PipelineCreateInfo.renderPass = m_pSwapChain->getRenderPass();
	PipelineCreateInfo.subpass = 0;
	PipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(m_pDevice->getDevice(), VK_NULL_HANDLE, 1, &PipelineCreateInfo, nullptr, &m_GraphicsPipeline) != VK_SUCCESS)
	{
		spdlog::error("failed to create graphics pipeline!");
		return false;
	}

	vkDestroyShaderModule(m_pDevice->getDevice(), FragShaderModule, nullptr);
	vkDestroyShaderModule(m_pDevice->getDevice(), VertShaderModule, nullptr);

	spdlog::info("created render pipeline");
	return true;
}

bool Lemon::CRenderPipeline::__readFile(const std::string& vFilename, std::vector<char>& voBuffer)
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
