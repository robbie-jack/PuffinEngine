#pragma once

#include "puffin/types/quat.h"

#include "puffin/math_helpers.h"

namespace puffin::maths
{
	struct EulerAngles
	{
		EulerAngles() : pitch(0.0f), yaw(0.0f), roll(0.0f) {}
		EulerAngles(const float& pitch, const float& yaw, const float& roll) : pitch(pitch), yaw(yaw), roll(roll) {}
		EulerAngles(const glm::vec3& vec) : pitch(vec.x), yaw(vec.y), roll(vec.z) {}
		EulerAngles(const Vector3f& vec) : pitch(vec.x), yaw(vec.y), roll(vec.z) {}

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

	inline EulerAngles deg_to_rad(const EulerAngles& euler)
	{
		return { deg_to_rad(euler.pitch), deg_to_rad(euler.yaw), deg_to_rad(euler.roll) };
	}

	inline EulerAngles rad_to_deg(const EulerAngles& euler)
	{
		return { rad_to_deg(euler.pitch), rad_to_deg(euler.yaw), rad_to_deg(euler.roll) };
	}

	// Convert from euler in degrees to quaternion in radians
	inline maths::Quat euler_to_quat(const maths::EulerAngles& euler)
	{
		const auto euler_rad = maths::deg_to_rad(euler);

		return { glm::quat(glm::vec3(euler_rad.pitch, euler_rad.yaw, euler_rad.roll)) };
	}

	// Convert from euler in degrees to quaternion in radians
	inline maths::EulerAngles quat_to_euler(const maths::Quat& quat)
	{
		return maths::rad_to_deg(maths::EulerAngles(glm::eulerAngles(glm::quat(quat))));
	}
}