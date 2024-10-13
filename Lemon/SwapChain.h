#pragma once

namespace Lemon
{
	class CDevice;
	class CSwapChain
	{
	public:
		bool init(const CDevice* vDevice, VkExtent2D vExtent);

		void cleanup();

		bool recreate(VkExtent2D vExtent);

		[[nodiscard]] VkFormat getSwapChainImageFormat() const { return m_SwapChainImageFormat; }

		[[nodiscard]] const VkExtent2D& getSwapChainExtent() const { return m_SwapChainExtent; }

		[[nodiscard]] uint32_t getMaxFramesInFlight() const { return m_MaxFramesInFlight; }

		[[nodiscard]] uint32_t getCurrentFrame() const { return m_CurrentFrame; }

		[[nodiscard]] VkRenderPass getRenderPass() const { return m_RenderPass; }

		[[nodiscard]] VkFramebuffer getFrameBuffer(size_t vIndex) const
		{
			return vIndex < m_SwapChainFramebuffers.size() ? m_SwapChainFramebuffers[vIndex] : VK_NULL_HANDLE;
		}

		VkResult acquireNextImage(uint32_t* voImageIndex) const;

		VkResult submitCommandBuffers(VkCommandBuffer vCommandBuffer, uint32_t vImageIndex);

	private:
		bool __createSwapChain(VkExtent2D vExtent);

		void __getSwapChainImages();

		bool __createImageViews();

		bool __createRenderPass();

		bool __createSwapChainFramebuffers();

		void __cleanupSwapChain();

		bool __createSyncObjects();

		void __cleanupSyncObjects();

		const CDevice* m_pDevice = nullptr;
		VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
		std::vector<VkImage> m_SwapChainImages;
		std::vector<VkImageView> m_SwapChainImageViews;
		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
		std::vector<VkFramebuffer> m_SwapChainFramebuffers;

		VkFormat m_SwapChainImageFormat{};
		VkExtent2D m_SwapChainExtent{};

		uint32_t m_MaxFramesInFlight = 2;
		uint32_t m_CurrentFrame = 0;
		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;
	};
}