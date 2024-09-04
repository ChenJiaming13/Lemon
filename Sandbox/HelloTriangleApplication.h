#pragma once

#include <chrono>
#include <vector>
#include <cstdint>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "core.h"

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<Lemon::SVertex> VERTICES = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> INDICES = {
	0, 1, 2, 2, 3, 0
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
	void __cleanupSwapChain() const;
	void __cleanup() const;
	void __recreateSwapChain();
	void __recordCommandBuffer(VkCommandBuffer vCommandBuffer, uint32_t vImageIndex) const;
	void __updateUniformBuffer(uint32_t vCurrentImage) const;
	void __drawFrame();

	GLFWwindow* m_pWindow = nullptr;

	VkInstance m_Instance = VK_NULL_HANDLE;
	VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_Device = VK_NULL_HANDLE;

	VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
	VkQueue m_PresentQueue = VK_NULL_HANDLE;

	VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
	std::vector<VkImage> m_SwapChainImages;
	VkFormat m_SwapChainImageFormat{};
	VkExtent2D m_SwapChainExtent{};
	std::vector<VkImageView> m_SwapChainImageViews;
	std::vector<VkFramebuffer> m_SwapChainFramebuffers;

	VkRenderPass m_RenderPass = VK_NULL_HANDLE;
	VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;

	VkCommandPool m_CommandPool = VK_NULL_HANDLE;

	VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory m_VertexDeviceMemory = VK_NULL_HANDLE;
	VkBuffer m_IndexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory m_IndexDeviceMemory = VK_NULL_HANDLE;

	std::vector<VkBuffer> m_UniformBuffers;
	std::vector<VkDeviceMemory> m_UniformDevicesMemory;
	std::vector<void*> m_UniformBuffersMapped;

	VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> m_DescriptorSets;

	std::vector<VkCommandBuffer> m_CommandBuffers;

	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	std::vector<VkFence> m_InFlightFences;
	uint32_t m_CurrentFrame = 0;

	bool m_IsFramebufferResized = false;
};
