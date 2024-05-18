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
			return { w, x, y, z };
		}

		Quat operator*=(const Quat& q)
		{
			float w1 = w;
			float x1 = x;
			float y1 = y;
			float z1 = z;

			float w2 = q.w;
			float x2 = q.x;
			float y2 = q.y;
			float z2 = q.z;

			w = w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2;
			x = w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2;
			y = w1 * y2 + y1 * w2 + z1 * x2 - x1 * z2;
			z = w1 * z2 + z1 * w2 + x1 * y2 - y1 * x2;
			return *this;
		}

		Quat operator*(const Quat& q) const
		{
			Quat quat;

			float w1 = w;
			float x1 = x;
			float y1 = y;
			float z1 = z;

			float w2 = q.w;
			float x2 = q.x;
			float y2 = q.y;
			float z2 = q.z;

			quat.w = w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2;
			quat.x = w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2;
			quat.y = w1 * y2 + y1 * w2 + z1 * x2 - x1 * z2;
			quat.z = w1 * z2 + z1 * w2 + x1 * y2 - y1 * x2;
			return quat;
		}

		float x, y, z, w;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Quat, x, y, z, w)
	};
}