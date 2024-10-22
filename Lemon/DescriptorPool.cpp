#include "pch.h"
#include "DescriptorPool.h"
#include "Device.h"

bool Lemon::CDescriptorPool::init(const CDevice* vDevice, uint32_t vMaxSets,
	const std::vector<VkDescriptorPoolSize>& vPoolSizes)
{
	_ASSERTE(vDevice != nullptr);
	m_pDevice = vDevice;
	VkDescriptorPoolCreateInfo PoolCreateInfo{};
	PoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	PoolCreateInfo.poolSizeCount = static_cast<uint32_t>(vPoolSizes.size());
	PoolCreateInfo.pPoolSizes = vPoolSizes.data();
	PoolCreateInfo.maxSets = vMaxSets;

	if (vkCreateDescriptorPool(m_pDevice->getDevice(), &PoolCreateInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
	{
		spdlog::error("failed to create descriptor pool!");
		return false;
	}
	spdlog::info("created descriptor pool");
	return true;
}

void Lemon::CDescriptorPool::cleanup()
{
	if (m_DescriptorPool == nullptr || m_pDevice == nullptr) return;
	vkDestroyDescriptorPool(m_pDevice->getDevice(), m_DescriptorPool, nullptr);
	m_DescriptorPool = VK_NULL_HANDLE;
}

bool Lemon::CDescriptorPool::allocateDescriptorSets(const std::vector<VkDescriptorSetLayout>& vSetLayouts,
													std::vector<VkDescriptorSet>& voSets) const
{
	VkDescriptorSetAllocateInfo SetAllocateInfo{};
	SetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	SetAllocateInfo.descriptorPool = m_DescriptorPool;
	SetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(vSetLayouts.size());
	SetAllocateInfo.pSetLayouts = vSetLayouts.data();

	voSets.resize(vSetLayouts.size());
	if (vkAllocateDescriptorSets(m_pDevice->getDevice(), &SetAllocateInfo, voSets.data()) != VK_SUCCESS)
	{
		spdlog::error("failed to allocate descriptor sets!");
		return false;
	}
	spdlog::info("allocated descriptor sets");
	return true;
}
