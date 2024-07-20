#pragma once

#define NOMINMAX
#include <Windows.h>
#include <minwindef.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <optional>
#include <vector>

struct SRequiredQueueFamilyIndices
{
	std::optional<uint32_t> _GraphicsFamily;
	std::optional<uint32_t> _PresentFamily;
	bool isComplete() const { return _GraphicsFamily.has_value() && _PresentFamily.has_value(); }
};

struct SSwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR _Capabilities;
	std::vector<VkSurfaceFormatKHR> _Formats;
	std::vector<VkPresentModeKHR> _PresentModes;
};

class CHelloTriangleApplication
{
public:
	CHelloTriangleApplication() = default;
	void run();

private:
	void __cleanUp();
	void __mainLoop();
	void __initWindow();
	void __initVulkan();
	void __createInstance();
	void __createSurface();
	void __setRequiredInstanceExtensions();
	void __setRequiredPhyDeviceExtensions();
	void __findQueueFamilies(const VkPhysicalDevice &vPhyDevice, SRequiredQueueFamilyIndices &voFamilyIndices) const;
	void __querySwapChainSupport(const VkPhysicalDevice& vPhyDevice, SSwapChainSupportDetails& voDetails) const;
	bool __isPhysicalDeviceSuitable(VkPhysicalDevice vPhyDevice);
	void __pickPhysicalDevice();
	void __createLogicalDevice();
	void __createSwapChain();
	void __createImageViews();

	int m_Width = 800, m_Height = 600;
	std::vector<const char*> m_RequiredInstanceExtensions;
	std::vector<const char*> m_RequiredPhyDeviceExtensions;
	GLFWwindow *m_pWindow = nullptr;
	VkInstance m_Instance{};
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_Device{};
	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;
	VkSurfaceKHR m_Surface;
	VkSwapchainKHR m_SwapChain;
	std::vector<VkImage> m_SwapChainImages;
	std::vector<VkImageView> m_SwapChainImageViews;
	VkFormat m_SwapChainImageFormat;
	VkExtent2D m_SwapChainExtent;
};
