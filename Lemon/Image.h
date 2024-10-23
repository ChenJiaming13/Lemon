#pragma once

namespace Lemon
{
	class CDevice;
	class CImage
	{
	public:
		bool init(const CDevice* vDevice, const char* vImagePath);

		void cleanup();

		void getDescriptorImageInfo(VkDescriptorImageInfo* voImageInfo) const;

	private:
		bool __createImage(VkExtent3D vImageExtent, VkFormat vFormat, VkImageTiling vTiling, VkImageUsageFlags vUsage, VkMemoryPropertyFlags vPropertyFlags);

		bool __transitionImageLayout(VkImageLayout vNewLayout);

		void __copyFromBuffer(VkBuffer vBuffer) const;

		bool __createImageView();

		bool __createImageSampler();

		const CDevice* m_pDevice = nullptr;
		VkImage m_Image = VK_NULL_HANDLE;
		VkDeviceMemory m_DeviceMemory = VK_NULL_HANDLE;
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkSampler m_ImageSampler = VK_NULL_HANDLE;
		VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;
		VkImageLayout m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkExtent3D m_ImageExtent = {0, 0, 0};
	};
}