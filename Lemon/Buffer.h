#pragma once

namespace Lemon
{
	class CDevice;
	class CBuffer
	{
	public:
		~CBuffer() { cleanup(); }

		bool init(const CDevice* vDevice, VkDeviceSize vBufferSize, VkBufferUsageFlags vUsageFlags, VkMemoryPropertyFlags vPropertyFlags);

		void cleanup();

		void copyFrom(const CBuffer* vSrcBuffer, const VkBufferCopy& vCopyRegion) const;

		void* mapMemory(VkDeviceSize vOffset, VkDeviceSize vBufferSize);

		[[nodiscard]] VkBuffer getBuffer() const { return m_Buffer; }

		[[nodiscard]] void* getMappedPtr() const { return m_pMapped; }

		void unmapMemory();

		void fillData(VkDeviceSize vOffset, VkDeviceSize vBufferSize, const void* vData);

		[[nodiscard]] VkDeviceSize getBufferSize() const { return m_BufferSize; }

	private:
		bool __findMemoryType(uint32_t vTypeFilter, VkMemoryPropertyFlags vPropertyFlags, uint32_t* voType) const;

		const CDevice* m_pDevice = nullptr;
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VkDeviceMemory m_DeviceMemory = VK_NULL_HANDLE;
		void* m_pMapped = nullptr;
		VkDeviceSize m_BufferSize = 0;
	};
}