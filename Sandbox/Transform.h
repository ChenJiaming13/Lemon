#pragma once

#include <glm/glm.hpp>

struct SUniformBufferObject
{
	glm::mat4 _Model;
	glm::mat4 _View;
	glm::mat4 _Projection;
};

class CTransform
{
public:
	static void dumpUniformBufferObject(uint32_t vWidth, uint32_t vHeight, SUniformBufferObject& voUBO);
};