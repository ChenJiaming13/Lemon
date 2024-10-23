#pragma once

namespace Lemon
{
	struct SQueueFamilyIndices
	{
		std::optional<uint32_t> _GraphicsFamily;
		std::optional<uint32_t> _PresentFamily;
	};

	struct SSwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR _Capabilities;
		std::vector<VkSurfaceFormatKHR> _Formats;
		std::vector<VkPresentModeKHR> _PresentModes;
	};

	class CGlfwWindow;
	class CInstance;
	class CDevice
	{
	public:
		bool init(const CGlfwWindow* vWindow);

		void cleanup();

		[[nodiscard]] VkSurfaceKHR getSurface() const { return m_Surface; }

		[[nodiscard]] VkPhysicalDevice getPhysicalDevice() const { return m_PhysicalDevice; }

		[[nodiscard]] VkDevice getDevice() const { return m_Device; }

		[[nodiscard]] VkQueue getGraphicsQueue() const { return m_GraphicsQueue; }

		[[nodiscard]] VkQueue getPresentQueue() const { return m_PresentQueue; }

		bool allocatePrimaryCommandBuffer(uint32_t vCommandBufferCount, VkCommandBuffer* voCommandBuffers) const;

		[[nodiscard]] VkCommandBuffer beginSingleTimeCommandBuffer() const;

		void endAndSubmitCommandBuffer(VkCommandBuffer vCommandBuffer) const;

		[[nodiscard]] const SQueueFamilyIndices& getQueueFamilyIndices() const { return m_QueueFamilyIndices; }

		void querySwapChainSupportDetails(SSwapChainSupportDetails& voDetails) const;

		bool queryMemoryType(uint32_t vTypeFilter, VkMemoryPropertyFlags vPropertyFlags, uint32_t* voType) const;

	private:
		bool __createInstance(const std::vector<const char*>& vInstanceExtensions);

		bool __setupDebugMessenger();

		bool __pickPhysicalDevice(const std::vector<const char*>& vDeviceExtensions);

		bool __createLogicalDevice(const std::vector<const char*>& vDeviceExtensions);

		bool __getQueue();

		bool __createCommandPool();

		static bool __checkValidationLayerSupport();

		static void __getValidationLayers(std::vector<const char*>& voValidationLayers);

		static void __getRequiredInstanceExtensions(std::vector<const char*>& voInstanceExtensions);

		static void __getRequiredDeviceExtensions(std::vector<const char*>& voDeviceExtensions);

		static void __populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& vDebugCreateInfo);

		static bool __isDeviceSuitable(VkPhysicalDevice vPhysicalDevice, VkSurfaceKHR vSurface, const std::vector<const char*>& vDeviceExtensions);

		static void __findQueueFamilyIndices(SQueueFamilyIndices& voQueueFamilyIndices, VkPhysicalDevice vPhysicalDevice, VkSurfaceKHR vSurface);

		static void __querySwapChainSupportDetails(SSwapChainSupportDetails& voDetails, VkPhysicalDevice vPhysicalDevice, VkSurfaceKHR vSurface);

		VkInstance m_Instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;
		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		VkQueue m_PresentQueue = VK_NULL_HANDLE;
		VkCommandPool m_CommandPool = VK_NULL_HANDLE;
		SQueueFamilyIndices m_QueueFamilyIndices{};

#ifdef _DEBUG
		static constexpr bool m_EnableValidationLayers = true;
#else
		static constexpr bool m_EnableValidationLayers = false;
#endif
	};
}
