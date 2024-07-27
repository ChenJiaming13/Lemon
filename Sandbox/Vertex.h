#pragma once

#include <array>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

struct SVertex
{
	glm::vec2 _Position;
	glm::vec3 _Color;

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
};
