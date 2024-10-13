#pragma once
#include "Device.h"

namespace Lemon
{
	struct SRenderPipelineCreateInfo
	{
		const char* _VertFilePath;
		const char* _FragFilePath;
		VkPipelineLayout _PipelineLayout;
	};

	class CDevice;
	class CSwapChain;
	class CRenderPipeline
	{
	public:
		~CRenderPipeline() { cleanup(); }

		bool init(const CDevice* vDevice, const CSwapChain* vSwapChain, const SRenderPipelineCreateInfo& vCreateInfo);

		void cleanup();

		[[nodiscard]] VkPipeline getGraphicsPipeline() const { return m_GraphicsPipeline; }

	private:
		bool __createDescriptorSetLayout();
		bool __createShaderModule(const std::string& vFilename, VkShaderModule* voShaderModule) const;
		bool __createGraphicsPipeline(const SRenderPipelineCreateInfo& vCreateInfo);

		static bool __readFile(const std::string& vFilename, std::vector<char>& voBuffer);

		const CDevice* m_pDevice = nullptr;
		const CSwapChain* m_pSwapChain = nullptr;
		VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
		VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;
	};
}
