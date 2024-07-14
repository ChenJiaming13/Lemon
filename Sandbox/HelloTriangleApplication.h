#pragma once

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
	void __dumpRequiredExtensions(std::vector<const char *> &voExtensions);
	void __findQueueFamilies(const VkPhysicalDevice &vPhyDevice, SRequiredQueueFamilyIndices &voFamilyIndices) const;
	void __pickPhysicalDevice();
	void __createLogicalDevice();

	GLFWwindow *m_pWindow = nullptr;
	VkInstance m_Instance{};
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_Device{};
	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;
	VkSurfaceKHR m_Surface;
};
