#pragma once

#include <chrono>
#include <vector>
#include <cstdint>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "DescriptorPool.h"
#include "GlfwWindow.h"
#include "Device.h"
#include "Mesh.h"
#include "RenderPipeline.h"
#include "SwapChain.h"

struct SModelViewProj
{
	glm::mat4 _Model;
	glm::mat4 _View;
	glm::mat4 _Proj;
};

class CHelloTriangleApplication
{
public:
	void run()
	{
		__init();
		__mainLoop();
		__cleanup();
	}

private:
	void __init();
	void __mainLoop();
	void __cleanup();
	void __recordCommandBuffer(VkCommandBuffer vCommandBuffer, uint32_t vImageIndex, uint32_t vCurrentFrame) const;
	void __drawFrame();
	void __recreateSwapChain();
	bool __createPipelineLayout();
	bool __createDescriptorSets();
	bool __createUniformBuffers();

	Lemon::CGlfwWindow m_Window;
	Lemon::CDevice m_Device;
	Lemon::CSwapChain m_SwapChain;
	Lemon::CRenderPipeline m_RenderPipeline;
	Lemon::CMesh m_Mesh;
	Lemon::CDescriptorPool m_DescriptorPool;
	VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> m_DescriptorSets;
	std::vector<const Lemon::CBuffer*> m_UniformBuffers;
	std::vector<VkCommandBuffer> m_CommandBuffers;
	bool m_IsFramebufferResized = false;
};
