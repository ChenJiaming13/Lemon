#include "Vertex.h"

VkVertexInputBindingDescription SVertex::getBindingDescription()
{
	VkVertexInputBindingDescription BindingDescription{};
	BindingDescription.binding = 0;
	BindingDescription.stride = sizeof(SVertex);
	BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return BindingDescription;
}

std::array<VkVertexInputAttributeDescription, 2> SVertex::getAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 2> AttributeDescriptions{};
	AttributeDescriptions[0].binding = 0;
	AttributeDescriptions[0].location = 0;
	AttributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	AttributeDescriptions[0].offset = offsetof(SVertex, _Position);
	AttributeDescriptions[1].binding = 0;
	AttributeDescriptions[1].location = 1;
	AttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	AttributeDescriptions[1].offset = offsetof(SVertex, _Color);
	return AttributeDescriptions;
}
