#pragma once

#include <chrono>
#include <vector>
#include <cstdint>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "GlfwWindow.h"
#include "Device.h"
#include "Mesh.h"
#include "RenderPipeline.h"
#include "SwapChain.h"

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
	void __recordCommandBuffer(VkCommandBuffer vCommandBuffer, uint32_t vImageIndex) const;
	void __drawFrame();
	void __recreateSwapChain();

	Lemon::CGlfwWindow m_Window;
	Lemon::CDevice m_Device;
	Lemon::CSwapChain m_SwapChain;
	Lemon::CRenderPipeline m_RenderPipeline;
	Lemon::CMesh m_Mesh;
	std::vector<VkCommandBuffer> m_CommandBuffers;
	bool m_IsFramebufferResized = false;
};
