#pragma once

#include "puffin/types/quat.h"

#include "puffin/math_helpers.h"

namespace puffin::maths
{
	struct EulerAngles
	{
		EulerAngles() : pitch(0.0f), yaw(0.0f), roll(0.0f) {}
		EulerAngles(float pitch, float yaw, float roll) : pitch(pitch), yaw(yaw), roll(roll) {}

		EulerAngles operator+(const EulerAngles& other) const
		{
			EulerAngles euler(pitch, yaw, roll);
			euler.pitch += other.pitch;
			euler.yaw += other.yaw;
			euler.roll += other.roll;

			return euler;
		}

		EulerAngles& operator+=(const EulerAngles& other)
		{
			pitch += other.pitch;
			yaw += other.yaw;
			roll += other.roll;

			return *this;
		}

		EulerAngles operator-(const EulerAngles& other) const
		{
			EulerAngles euler(pitch, yaw, roll);
			euler.pitch -= other.pitch;
			euler.yaw -= other.yaw;
			euler.roll -= other.roll;

			return euler;
		}

		float pitch, yaw, roll;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(EulerAngles, pitch, yaw, roll)
	};

	static EulerAngles deg_to_rad(EulerAngles euler_angles)
	{
		return EulerAngles(deg_to_rad(euler_angles.pitch), deg_to_rad(euler_angles.yaw), deg_to_rad(euler_angles.roll));
	}
}