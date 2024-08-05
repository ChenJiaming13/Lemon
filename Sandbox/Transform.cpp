#include "Transform.h"
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

void CTransform::dumpUniformBufferObject(uint32_t vWidth, uint32_t vHeight, SUniformBufferObject& voUBO)
{
	static auto StartTime = std::chrono::high_resolution_clock::now();
	const auto CurrTime = std::chrono::high_resolution_clock::now();
	const float Time = std::chrono::duration<float, std::chrono::seconds::period>(CurrTime - StartTime).count();
	voUBO._Model = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f) * 5.0f);
	voUBO._Model = glm::rotate(voUBO._Model, Time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	voUBO._View = glm::lookAt(
		glm::vec3(2.0f, 2.0f, 2.0f) * 5.0f, // eye
		glm::vec3(0.0f, 0.0f, 0.0f), // focus
		glm::vec3(0.0f, 0.0f, 1.0f)  // up
	);
	voUBO._Projection = glm::perspective(
		glm::radians(45.0f),
		static_cast<float>(vWidth) / static_cast<float>(vHeight),
		0.1f,
		100.0f
	);
	voUBO._Projection[1][1] *= -1.0f;
	//spdlog::debug("rotate: {}", Time * glm::radians(90.0f));
}
