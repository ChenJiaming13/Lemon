#include "pch.h"
#include "Buffer.h"
#include "Device.h"

bool Lemon::CBuffer::init(const CDevice* vDevice, VkDeviceSize vBufferSize, VkBufferUsageFlags vUsageFlags, VkMemoryPropertyFlags vPropertyFlags)
{
	m_pDevice = vDevice;
	m_BufferSize = vBufferSize;

	VkBufferCreateInfo BufferCreateInfo{};
	BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	BufferCreateInfo.size = vBufferSize;
	BufferCreateInfo.usage = vUsageFlags;
	BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(m_pDevice->getDevice(), &BufferCreateInfo, nullptr, &m_Buffer) != VK_SUCCESS)
	{
		spdlog::error("failed to create voBuffer!");
		return false;
	}

	VkMemoryRequirements MemoryRequirements;
	vkGetBufferMemoryRequirements(m_pDevice->getDevice(), m_Buffer, &MemoryRequirements);

	VkMemoryAllocateInfo AllocateInfo{};
	AllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	AllocateInfo.allocationSize = MemoryRequirements.size;
	if (!__findMemoryType(MemoryRequirements.memoryTypeBits, vPropertyFlags, &AllocateInfo.memoryTypeIndex))
	{
		spdlog::error("failed to find suitable memory type!");
		return false;
	}

	if (vkAllocateMemory(m_pDevice->getDevice(), &AllocateInfo, nullptr, &m_DeviceMemory) != VK_SUCCESS)
	{
		spdlog::error("failed to allocate voBuffer memory!");
		return false;
	}

	vkBindBufferMemory(m_pDevice->getDevice(), m_Buffer, m_DeviceMemory, 0);

	return true;
}

void Lemon::CBuffer::cleanup()
{
	vkDestroyBuffer(m_pDevice->getDevice(), m_Buffer, nullptr);
	vkFreeMemory(m_pDevice->getDevice(), m_DeviceMemory, nullptr);
	m_Buffer = VK_NULL_HANDLE;
	m_DeviceMemory = VK_NULL_HANDLE;
}

void Lemon::CBuffer::copyFrom(const CBuffer* vSrcBuffer, const VkBufferCopy& vCopyRegion) const
{
	const VkCommandBuffer CommandBuffer = m_pDevice->beginSingleTimeCommandBuffer();
	vkCmdCopyBuffer(CommandBuffer, vSrcBuffer->m_Buffer, m_Buffer, 1, &vCopyRegion);
	m_pDevice->endAndSubmitCommandBuffer(CommandBuffer);
}

void* Lemon::CBuffer::mapMemory(VkDeviceSize vOffset, VkDeviceSize vBufferSize)
{
	if (m_pMapped != nullptr) return nullptr;
	vkMapMemory(m_pDevice->getDevice(), m_DeviceMemory, vOffset, vBufferSize, 0, &m_pMapped);
	return m_pMapped;
}

void Lemon::CBuffer::unmapMemory()
{
	vkUnmapMemory(m_pDevice->getDevice(), m_DeviceMemory);
	m_pMapped = nullptr;
}

void Lemon::CBuffer::fillData(VkDeviceSize vOffset, VkDeviceSize vBufferSize, const void* vData)
{
	void* pData = mapMemory(vOffset, vBufferSize);
	memcpy(pData, vData, vBufferSize);
	unmapMemory();
}

bool Lemon::CBuffer::__findMemoryType(uint32_t vTypeFilter, VkMemoryPropertyFlags vPropertyFlags, uint32_t* voType) const
{
	VkPhysicalDeviceMemoryProperties MemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(m_pDevice->getPhysicalDevice(), &MemoryProperties);
	for (uint32_t i = 0; i < MemoryProperties.memoryTypeCount; i++)
	{
		if ((vTypeFilter & (1 << i)) && (MemoryProperties.memoryTypes[i].propertyFlags & vPropertyFlags) == vPropertyFlags)
		{
			*voType = i;
			return true;
		}
	}
	spdlog::error("failed to find suitable memory type!");
	return false;
}
