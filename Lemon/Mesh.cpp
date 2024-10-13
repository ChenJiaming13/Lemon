#include "pch.h"
#include "Mesh.h"
#include "Buffer.h"

void Lemon::CMesh::SVertex::getVertexDescription(VkVertexInputBindingDescription& voBindingDescription,
                                                 std::array<VkVertexInputAttributeDescription, 2>& voAttributeDescriptions)
{
	voBindingDescription.binding = 0;
	voBindingDescription.stride = sizeof(SVertex);
	voBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	voAttributeDescriptions[0].binding = 0;
	voAttributeDescriptions[0].location = 0;
	voAttributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	voAttributeDescriptions[0].offset = offsetof(SVertex, _Pos);

	voAttributeDescriptions[1].binding = 0;
	voAttributeDescriptions[1].location = 1;
	voAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	voAttributeDescriptions[1].offset = offsetof(SVertex, _Color);
}

bool Lemon::CMesh::init(const CDevice* vDevice, const std::vector<SVertex>& vVertices, const std::vector<uint16_t>& vIndices)
{
	m_pDevice = vDevice;
	m_pVertexBuffer = __createAndFillBuffer(sizeof(vVertices[0]) * vVertices.size(), vVertices.data(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	if (m_pVertexBuffer == nullptr)
	{
		spdlog::error("failed to create vertex buffer!");
		return false;
	}
	m_pIndexBuffer = __createAndFillBuffer(sizeof(vIndices[0]) * vIndices.size(), vIndices.data(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	if (m_pIndexBuffer == nullptr)
	{
		spdlog::error("failed to create index buffer!");
		return false;
	}
	m_IndexCount = static_cast<uint32_t>(vIndices.size());
	return true;
}

void Lemon::CMesh::cleanup()
{
	delete m_pVertexBuffer;
	m_pVertexBuffer = nullptr;
	delete m_pIndexBuffer;
	m_pIndexBuffer = nullptr;
}

void Lemon::CMesh::draw(VkCommandBuffer vCommandBuffer) const
{
	const VkBuffer VertexBuffers[] = { m_pVertexBuffer->getBuffer() };
	constexpr VkDeviceSize Offsets[] = { 0 };
	vkCmdBindVertexBuffers(vCommandBuffer, 0, 1, VertexBuffers, Offsets);
	vkCmdBindIndexBuffer(vCommandBuffer, m_pIndexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(vCommandBuffer, m_IndexCount, 1, 0, 0, 0);
}

Lemon::CBuffer* Lemon::CMesh::__createAndFillBuffer(VkDeviceSize vBufferSize, const void* vBufferData, VkMemoryPropertyFlags vBufferUsage) const
{
	CBuffer StagingBuffer;
	if (!StagingBuffer.init(m_pDevice, vBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
		return nullptr;
	StagingBuffer.fillData(0, vBufferSize, vBufferData);

	const auto pBuffer = new CBuffer;
	if (!pBuffer->init(m_pDevice, vBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | vBufferUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
	{
		delete pBuffer;
		return nullptr;
	}
	pBuffer->copyFrom(&StagingBuffer, { 0, 0, vBufferSize });

	StagingBuffer.cleanup();
	return pBuffer;
}
