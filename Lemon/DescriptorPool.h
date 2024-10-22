#pragma once

namespace Lemon
{
	class CDevice;
	class CDescriptorPool
	{
	public:
		~CDescriptorPool() { cleanup(); }

		bool init(const CDevice* vDevice, uint32_t vMaxSets, const std::vector<VkDescriptorPoolSize>& vPoolSizes);

		void cleanup();

		bool allocateDescriptorSets(const std::vector<VkDescriptorSetLayout>& vSetLayouts, std::vector<VkDescriptorSet>& voSets) const;

	private:
		const CDevice* m_pDevice = nullptr;
		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
	};
}
