#pragma once

#include <functional>

namespace Lemon
{
	class CGlfwWindow
	{
	public:
		bool init(int vWidth, int vHeight, const char* vTitle);

		void cleanup();

		void getFrameBufferSize(int* voWidth, int* voHeight) const;

		bool createSurface(VkInstance vInstance, VkSurfaceKHR* voSurface) const;

		[[nodiscard]] bool shouldCloseWindow() const { return glfwWindowShouldClose(m_pWindow); }

		void addFrameBufferSizeCallback(const std::function<void(int, int)>& vCallback) { m_FrameBufferSizeCallback.push_back(vCallback); }

		static void pollEvents() { glfwPollEvents(); }

		static void getRequiredInstanceExtensions(std::vector<const char*>& voInstanceExtensions);

	private:
		void __registerCallbacks();

		GLFWwindow* m_pWindow = nullptr;
		std::vector<std::function<void(int, int)>> m_FrameBufferSizeCallback;
	};
}