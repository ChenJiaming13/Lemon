#include "pch.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "Image.h"
#include "Buffer.h"
#include "Device.h"

bool Lemon::CImage::init(const CDevice* vDevice, const char* vImagePath)
{
	m_pDevice = vDevice;
	int Width, Height, Channels;
	stbi_uc* pPixels = stbi_load(vImagePath, &Width, &Height, &Channels, STBI_rgb_alpha);
	const VkDeviceSize ImageSize = static_cast<VkDeviceSize>(Width * Height * 4);

	if (!pPixels)
	{
		spdlog::error("failed to load texture: {}", vImagePath);
		return false;
	}

	CBuffer StagingBuffer;
	StagingBuffer.init(m_pDevice, ImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	StagingBuffer.fillData(0, ImageSize, pPixels);
	stbi_image_free(pPixels);

	if (!__createImage(
		{ static_cast<uint32_t>(Width), static_cast<uint32_t>(Height), 1 },
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	)) return false;

	if (!__transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL))
		return false;
	__copyFromBuffer(StagingBuffer.getBuffer());
	if (!__transitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL))
		return false;

	StagingBuffer.cleanup();
	if (!__createImageView()) return false;
	if (!__createImageSampler()) return false;
	return true;
}

void Lemon::CImage::cleanup()
{
	vkDestroySampler(m_pDevice->getDevice(), m_ImageSampler, nullptr);
	m_ImageSampler = VK_NULL_HANDLE;
	vkDestroyImageView(m_pDevice->getDevice(), m_ImageView, nullptr);
	m_ImageView = VK_NULL_HANDLE;
	vkDestroyImage(m_pDevice->getDevice(), m_Image, nullptr);
	m_Image = VK_NULL_HANDLE;
	vkFreeMemory(m_pDevice->getDevice(), m_DeviceMemory, nullptr);
	m_DeviceMemory = VK_NULL_HANDLE;
}

void Lemon::CImage::getDescriptorImageInfo(VkDescriptorImageInfo* voImageInfo) const
{
	_ASSERTE(voImageInfo != nullptr);
	voImageInfo->sampler = m_ImageSampler;
	voImageInfo->imageLayout = m_ImageLayout;
	voImageInfo->imageView = m_ImageView;
}

bool Lemon::CImage::__createImage(VkExtent3D vImageExtent, VkFormat vFormat, VkImageTiling vTiling, VkImageUsageFlags vUsage, VkMemoryPropertyFlags vPropertyFlags)
{
	VkImageCreateInfo ImageCreateInfo{};
	ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	ImageCreateInfo.extent = vImageExtent;
	ImageCreateInfo.mipLevels = 1;
	ImageCreateInfo.arrayLayers = 1;
	ImageCreateInfo.format = vFormat;
	ImageCreateInfo.tiling = vTiling;
	ImageCreateInfo.initialLayout = m_ImageLayout;
	ImageCreateInfo.usage = vUsage;
	ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(m_pDevice->getDevice(), &ImageCreateInfo, nullptr, &m_Image) != VK_SUCCESS)
	{
		spdlog::error("failed to create image!");
		return false;
	}
	m_ImageFormat = vFormat;
	m_ImageExtent = vImageExtent;

	VkMemoryRequirements MemoryRequirements;
	vkGetImageMemoryRequirements(m_pDevice->getDevice(), m_Image, &MemoryRequirements);

	VkMemoryAllocateInfo AllocateInfo{};
	AllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	AllocateInfo.allocationSize = MemoryRequirements.size;
	if (!m_pDevice->queryMemoryType(MemoryRequirements.memoryTypeBits, vPropertyFlags, &AllocateInfo.memoryTypeIndex))
		return false;
	if (vkAllocateMemory(m_pDevice->getDevice(), &AllocateInfo, nullptr, &m_DeviceMemory) != VK_SUCCESS)
	{
		spdlog::error("failed to allocate image memory!");
		return false;
	}

	vkBindImageMemory(m_pDevice->getDevice(), m_Image, m_DeviceMemory, 0);
	return true;
}

bool Lemon::CImage::__transitionImageLayout(VkImageLayout vNewLayout)
{
	const VkCommandBuffer CommandBuffer = m_pDevice->beginSingleTimeCommandBuffer();

	VkImageMemoryBarrier Barrier{};
	Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	Barrier.oldLayout = m_ImageLayout;
	Barrier.newLayout = vNewLayout;
	Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.image = m_Image;
	Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	Barrier.subresourceRange.baseMipLevel = 0;
	Barrier.subresourceRange.levelCount = 1;
	Barrier.subresourceRange.baseArrayLayer = 0;
	Barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags SourceStage;
	VkPipelineStageFlags DestinationStage;

	if (m_ImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && vNewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		Barrier.srcAccessMask = 0;
		Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		SourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		DestinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (m_ImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && vNewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		SourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		DestinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		spdlog::error("unsupported layout transition!");
		return false;
	}

	vkCmdPipelineBarrier(
		CommandBuffer,
		SourceStage, DestinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &Barrier
	);

	m_pDevice->endAndSubmitCommandBuffer(CommandBuffer);
	m_ImageLayout = vNewLayout;
	return true;
}

void Lemon::CImage::__copyFromBuffer(VkBuffer vBuffer) const
{
	const VkCommandBuffer CommandBuffer = m_pDevice->beginSingleTimeCommandBuffer();

	VkBufferImageCopy CopyRegion;
	CopyRegion.bufferOffset = 0;
	CopyRegion.bufferRowLength = 0;
	CopyRegion.bufferImageHeight = 0;
	CopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	CopyRegion.imageSubresource.mipLevel = 0;
	CopyRegion.imageSubresource.baseArrayLayer = 0;
	CopyRegion.imageSubresource.layerCount = 1;
	CopyRegion.imageOffset = { 0, 0, 0 };
	CopyRegion.imageExtent = m_ImageExtent;
	vkCmdCopyBufferToImage(CommandBuffer, vBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &CopyRegion);

	m_pDevice->endAndSubmitCommandBuffer(CommandBuffer);
}

bool Lemon::CImage::__createImageView()
{
	_ASSERTE(m_Image != VK_NULL_HANDLE);

	VkImageViewCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	CreateInfo.image = m_Image;
	CreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	CreateInfo.format = m_ImageFormat;
	CreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	CreateInfo.subresourceRange.baseMipLevel = 0;
	CreateInfo.subresourceRange.levelCount = 1;
	CreateInfo.subresourceRange.baseArrayLayer = 0;
	CreateInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(m_pDevice->getDevice(), &CreateInfo, nullptr, &m_ImageView) != VK_SUCCESS)
	{
		spdlog::error("failed to create texture image view!");
		return false;
	}
	spdlog::info("created texture image view");
	return true;
}

bool Lemon::CImage::__createImageSampler()
{
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(m_pDevice->getPhysicalDevice(), &properties);

	VkSamplerCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	CreateInfo.magFilter = VK_FILTER_LINEAR;
	CreateInfo.minFilter = VK_FILTER_LINEAR;
	CreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	CreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	CreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	CreateInfo.anisotropyEnable = VK_TRUE;
	CreateInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	CreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	CreateInfo.unnormalizedCoordinates = VK_FALSE;
	CreateInfo.compareEnable = VK_FALSE;
	CreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	CreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	if (vkCreateSampler(m_pDevice->getDevice(), &CreateInfo, nullptr, &m_ImageSampler) != VK_SUCCESS)
	{
		spdlog::error("failed to create texture sampler!");
		return false;
	}
	spdlog::info("created texture image sampler");
	return true;
}
