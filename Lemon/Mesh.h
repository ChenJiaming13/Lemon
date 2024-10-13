#pragma once

#include <glm/glm.hpp>

namespace Lemon
{
	class CDevice;
	class CBuffer;
	class CMesh
	{
	public:
		struct SVertex
		{
			glm::vec2 _Pos;
			glm::vec3 _Color;

			static void getVertexDescription(VkVertexInputBindingDescription& voBindingDescription,
				std::array<VkVertexInputAttributeDescription, 2>& voAttributeDescriptions);
		};

		~CMesh() { cleanup(); }

		bool init(const CDevice* vDevice, const std::vector<SVertex>& vVertices, const std::vector<uint16_t>& vIndices);

		void cleanup();

		void draw(VkCommandBuffer vCommandBuffer) const;

	private:
		CBuffer* __createAndFillBuffer(VkDeviceSize vBufferSize, const void* vBufferData, VkMemoryPropertyFlags vBufferUsage) const;

		const CDevice* m_pDevice = nullptr;
		CBuffer* m_pVertexBuffer = nullptr;
		CBuffer* m_pIndexBuffer = nullptr;
		uint32_t m_IndexCount = 0;
	};
}
