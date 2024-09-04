#pragma once

#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

struct GLFWwindow;

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

	struct SUniformBufferObject
	{
		alignas(16) glm::mat4 _Model;
		alignas(16) glm::mat4 _View;
		alignas(16) glm::mat4 _Proj;
	};

	struct SVertex
	{
		glm::vec2 _Pos;
		glm::vec3 _Color;
	};

	GLFWwindow* createWindow(int vWidth, int vHeight, const std::string& vTitle);

	bool createInstance(VkInstance& voInstance, const std::vector<const char*>& vInstanceExtensions);

	bool createSurface(VkSurfaceKHR& voSurface, const VkInstance& vInstance, GLFWwindow* vWindow);

	bool pickPhysicalDevice(
		VkPhysicalDevice& voPhysicalDevice,
		const VkInstance& vInstance,
		const VkSurfaceKHR& vSurface,
		const std::vector<const char*>& vDeviceExtensions
	);

	bool createLogicalDeviceAndQueues(
		VkDevice& voDevice,
		VkQueue& voGraphicsQueue,
		VkQueue& voPresentQueue,
		const VkPhysicalDevice& vPhysicalDevice,
		const VkSurfaceKHR& vSurface,
		const std::vector<const char*>& vDeviceExtensions
	);

	bool createCommandPool(
		VkCommandPool& voCommandPool,
		const VkDevice& vDevice,
		const VkPhysicalDevice& vPhysicalDevice,
		const VkSurfaceKHR& vSurface
	);

	bool allocateCommandBuffers(
		std::vector<VkCommandBuffer>& voCommandBuffers,
		const VkDevice& vDevice,
		const VkCommandPool& vCommandPool,
		size_t vAllocateCount
	);

	bool createSwapChain(
		VkSwapchainKHR& voSwapChain,
		VkFormat& voSwapChainImageFormat,
		VkExtent2D& voSwapChainExtent,
		const VkDevice& vDevice,
		const VkPhysicalDevice& vPhysicalDevice,
		const VkSurfaceKHR& vSurface,
		int vWidth, int vHeight
	);

	void getSwapChainImages(
		std::vector<VkImage>& voSwapChainImages,
		const VkDevice& vDevice,
		const VkSwapchainKHR& vSwapchain
	);

	bool createImageViews(
		std::vector<VkImageView>& voSwapChainImageViews,
		const VkDevice& vDevice,
		const std::vector<VkImage>& vSwapChainImages,
		const VkFormat& vSwapChainImageFormat
	);

	bool createRenderPass(
		VkRenderPass& voRenderPass,
		const VkDevice& vDevice,
		const VkFormat& vSwapChainImageFormat
	);

	bool createFramebuffers(
		std::vector<VkFramebuffer>& voSwapChainFrameBuffers,
		const VkDevice& vDevice,
		const std::vector<VkImageView>& vSwapChainImageViews,
		const VkRenderPass& vRenderPass,
		const VkExtent2D& vSwapChainExtent
	);

	bool createDescriptorPool(VkDescriptorPool& voDescriptorPool, const VkDevice& vDevice, size_t vMaxFramesInFlight);

	bool createDescriptorSetLayout(VkDescriptorSetLayout& voDescriptorSetLayout, const VkDevice& vDevice);

	bool allocateDescriptorSets(
		std::vector<VkDescriptorSet>& voDescriptorSets,
		const VkDevice& vDevice,
		const VkDescriptorPool& vDescriptorPool,
		const VkDescriptorSetLayout& vDescriptorSetLayout,
		int vMaxFramesInFlight
	);

	void updateDescriptorSets(
		const VkDevice& vDevice,
		const std::vector<VkDescriptorSet>& vDescriptorSets,
		const std::vector<VkBuffer>& vUniformBuffers,
		int vMaxFramesInFlight
	);

	bool createGraphicsPipeline(
		VkPipeline& voPipeline,
		VkPipelineLayout& voPipelineLayout,
		const VkDevice& vDevice,
		const VkRenderPass& vRenderPass,
		const VkDescriptorSetLayout& vDescriptorSetLayout,
		const std::string& vVertFilePath,
		const std::string& vFragFilePath
	);

	bool createVertexBuffer(
		VkBuffer& voVertexBuffer, VkDeviceMemory& voVertexDeviceMemory,
		const std::vector<SVertex>& vVertices,
		const VkDevice& vDevice,
		const VkPhysicalDevice& vPhysicalDevice,
		const VkCommandPool& vCommandPool,
		const VkQueue& vGraphicsQueue
	);

	bool createIndexBuffer(
		VkBuffer& voIndexBuffer, VkDeviceMemory& voIndexDeviceMemory,
		const std::vector<uint16_t>& vIndices,
		const VkDevice& vDevice,
		const VkPhysicalDevice& vPhysicalDevice,
		const VkCommandPool& vCommandPool,
		const VkQueue& vGraphicsQueue
	);

	bool createUniformBuffers(
		std::vector<VkBuffer>& voUniformBuffers,
		std::vector<VkDeviceMemory>& voUniformDeviceMemories,
		std::vector<void*>& voUniformBuffersMapped,
		const VkDevice& vDevice,
		const VkPhysicalDevice& vPhysicalDevice,
		int vMaxFramesInFlight
	);

	bool createSyncObjects(
		std::vector<VkSemaphore>& voImageAvailableSemaphores,
		std::vector<VkSemaphore>& voRenderFinishedSemaphores,
		std::vector<VkFence>& voInFlightFences,
		const VkDevice& vDevice,
		int vMaxFramesInFlight
	);
	///////////////////////////// Helper Functions /////////////////////////////

	void getRequiredInstanceExtensions(std::vector<const char*>& voInstanceExtensions);

	void getRequiredDeviceExtensions(std::vector<const char*>& voDeviceExtensions);

	void findQueueFamilyIndices(SQueueFamilyIndices& voQueueFamilyIndices, const VkPhysicalDevice& vPhysicalDevice, const VkSurfaceKHR& vSurface);

	void querySwapChainSupportDetails(SSwapChainSupportDetails& voDetails, const VkPhysicalDevice& vPhysicalDevice, const VkSurfaceKHR& vSurface);

	// Only For Lemon::pickPhysicalDevice
	// 所需队列要全部支持；所需设备扩展要全部支持；交换链要支持
	bool isDeviceSuitable(const VkPhysicalDevice& vPhysicalDevice, const VkSurfaceKHR& vSurface, const std::vector<const char*>& vDeviceExtensions);

	bool readFile(std::vector<char>& voBuffer, const std::string& vFilename);

	bool createShaderModule(
		VkShaderModule& voShaderModule,
		const std::string& vFilename,
		const VkDevice& vDevice
	);

	void getVertexDescription(
		VkVertexInputBindingDescription& voBindingDescription,
		std::array<VkVertexInputAttributeDescription, 2>& voAttributeDescriptions
	);

	bool findMemoryType(
		uint32_t& voType,
		uint32_t vTypeFilter,
		const VkPhysicalDevice& vPhysicalDevice,
		const VkMemoryPropertyFlags& vPropertyFlags
	);

	bool createBuffer(
		VkBuffer& voBuffer, VkDeviceMemory& voDeviceMemory,
		const VkDevice& vDevice,
		const VkPhysicalDevice& vPhysicalDevice,
		const VkDeviceSize& vBufferSize,
		const VkBufferUsageFlags& vUsageFlags,
		const VkMemoryPropertyFlags& vPropertyFlags
	);

	void copyBuffer(
		const VkDevice& vDevice,
		const VkCommandPool& vCommandPool,
		const VkQueue& vGraphicsQueue,
		const VkBuffer& vSrcBuffer, const VkBuffer& vDstBuffer,
		const VkDeviceSize& vBufferSize
	);
}