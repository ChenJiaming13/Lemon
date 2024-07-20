#include "Shader.h"
#include <fstream>
#include <spdlog/spdlog.h>

void CShader::readFile(const std::string& vFileName, std::vector<char>& voBuffer)
{
	voBuffer.clear();
	std::ifstream File(vFileName, std::ios::ate | std::ios::binary);
	if (!File.is_open())
	{
		spdlog::error("failed to open file: {}", vFileName);
		return;
	}
	size_t FileSize = (size_t)File.tellg();
	voBuffer.resize(FileSize);
	File.seekg(0);
	File.read(voBuffer.data(), FileSize);
	File.close();
	spdlog::info("read shader file: {}", vFileName);
}
