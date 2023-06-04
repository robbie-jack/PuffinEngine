#pragma once

#include "nlohmann/json.hpp"

#include <glm/gtc/quaternion.hpp>

namespace puffin::maths
{
	/*
	====================
	Quat
	====================
	*/
	struct Quat
	{
		Quat() : x(0.0f), y(0.0f), z(1.0f), w(0.0f) {}

		Quat(const Quat& quat) : x(quat.x), y(quat.y), z(quat.z), w(quat.w) {}

		Quat(float inX, float inY, float inZ, float inW = 1.0f) : x(inX), y(inY), z(inZ), w(inW) {}

		Quat(glm::quat quat) : x(quat.x), y(quat.y), z(quat.z), w(quat.w) {}

		explicit operator glm::quat() const
		{
			return {w, x, y, z};
		}

		float x, y, z, w;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Quat, x, y, z, w)
	};
}