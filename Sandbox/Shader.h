#pragma once

#include <vector>
#include <string>

class CShader
{
public:
	static void readFile(const std::string& vFileName, std::vector<char>& voBuffer);
};