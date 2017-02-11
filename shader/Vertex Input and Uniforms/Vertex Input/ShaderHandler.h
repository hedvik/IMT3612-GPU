#pragma once
#include <fstream>
#include <vector>

class ShaderHandler {
public:
	static std::vector<char> readFile(const std::string& filename);
};

