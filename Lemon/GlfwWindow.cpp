#include "pch.h"
#include "GlfwWindow.h"

bool Lemon::CGlfwWindow::init(int vWidth, int vHeight, const char* vTitle)
{
	if (m_pWindow != nullptr) return false;
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	m_pWindow = glfwCreateWindow(vWidth, vHeight, vTitle, nullptr, nullptr);
	if (m_pWindow == nullptr)
	{
		spdlog::error("failed to create window");
		glfwTerminate();
		return false;
	}
	__registerCallbacks();
	return true;
}

void Lemon::CGlfwWindow::cleanup()
{
	m_FrameBufferSizeCallback.clear();
	if (m_pWindow != nullptr)
	{
		glfwDestroyWindow(m_pWindow);
		m_pWindow = nullptr;
	}
	glfwTerminate();
}

void Lemon::CGlfwWindow::getFrameBufferSize(int* voWidth, int* voHeight) const
{
	*voWidth = 0;
	*voHeight = 0;
	glfwGetFramebufferSize(m_pWindow, voWidth, voHeight);
	while (*voWidth == 0 || *voHeight == 0)
	{
		glfwGetFramebufferSize(m_pWindow, voWidth, voHeight);
		glfwWaitEvents();
	}
}

bool Lemon::CGlfwWindow::createSurface(VkInstance vInstance, VkSurfaceKHR* voSurface) const
{
	if (vInstance == VK_NULL_HANDLE)
	{
		spdlog::error("VkInstance is nullptr!");
		return false;
	}
	if (glfwCreateWindowSurface(vInstance, m_pWindow, nullptr, voSurface) != VK_SUCCESS)
	{
		spdlog::error("failed to create window surface!");
		return false;
	}
	spdlog::info("created surface");
	return true;
}

void Lemon::CGlfwWindow::getRequiredInstanceExtensions(std::vector<const char*>& voInstanceExtensions)
{
	uint32_t GlfwExtensionCount = 0;
	const char** GlfwExtensions = glfwGetRequiredInstanceExtensions(&GlfwExtensionCount);
	voInstanceExtensions = std::vector(GlfwExtensions, GlfwExtensions + GlfwExtensionCount);
}

void Lemon::CGlfwWindow::__registerCallbacks()
{
	glfwSetWindowUserPointer(m_pWindow, this);
	glfwSetFramebufferSizeCallback(m_pWindow, [](GLFWwindow* vWindow, int vWidth, int vHeight) {
		for (const auto pWindow = static_cast<CGlfwWindow*>(glfwGetWindowUserPointer(vWindow)); const auto& FrameBufferSizeCallback : pWindow->m_FrameBufferSizeCallback)
			FrameBufferSizeCallback(vWidth, vHeight);
	});
}
