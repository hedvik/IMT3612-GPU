#pragma once
#include <glm\glm.hpp>

namespace Utilities {
	static float smoothInterpolation(float baseValue, float targetValue, float t) {
		return baseValue * pow(cos(t), 2) + (targetValue)* pow(sin(t), 2);
	}
}