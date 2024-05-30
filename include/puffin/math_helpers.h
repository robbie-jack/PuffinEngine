#pragma once

#include <algorithm>
#include <cmath>

#include "puffin/types/vector.h"
#include "puffin/types/quat.h"

#define PI 3.141592653589793238463

namespace puffin::maths
{
	template<typename T>
	static T clamp(const T& Value, const T& Min, const T& Max)
	{
		return std::max(Min, std::min(Max, Value));
	}

	static Vector2f clamp(const Vector2f& Vector, const Vector2f& Min, const Vector2f& Max)
	{
		return Vector2f(clamp(Vector.x, Min.x, Max.x), clamp(Vector.y, Min.y, Max.y));
	}

	static float deg_to_rad(const float degrees)
	{
		return degrees * (PI / 180.0);
	}

	static float rad_to_deg(const float radians)
	{
		return radians * (180.0 / PI);
	}

	static double deg_to_rad(const double degrees)
	{
		return degrees * (PI / 180.0);
	}

	static double rad_to_deg(const double radians)
	{
		return radians * (180.0 / PI);
	}

	static Vector3f deg_to_rad(const Vector3f& vecD)
	{
		Vector3f vecR;

		vecR.x = deg_to_rad(vecD.x);
		vecR.y = deg_to_rad(vecD.y);
		vecR.z = deg_to_rad(vecD.z);

		return vecR;
	}

	static Vector3f rad_to_deg(const Vector3f& vecR)
	{
		Vector3f vecD;

		vecD.x = rad_to_deg(vecR.x);
		vecD.y = rad_to_deg(vecR.y);
		vecD.z = rad_to_deg(vecR.z);

		return vecD;
	}

	static Vector2f rotate_point_around_point(const Vector2f inPoint, const Vector2f rotatePoint, const double inAngle)
	{
		Vector2f newPoint;
		const double angleRad = deg_to_rad(inAngle);

		newPoint.x = ((inPoint.x - rotatePoint.x) * std::cos(angleRad)) - ((inPoint.y - rotatePoint.y) * std::sin(angleRad)) + rotatePoint.x;
		newPoint.y = ((inPoint.y - rotatePoint.y) * std::sin(angleRad)) + ((inPoint.y - rotatePoint.y) * std::cos(angleRad)) + rotatePoint.y;

		return newPoint;
	}

	// Rotate Point around Origin by Angle
	static Vector2f rotate_point_around_origin(const Vector2f& inPoint, const double inAngle)
	{
		return rotate_point_around_point(inPoint, Vector2f(0.0f), inAngle);
	}

	static float lerp(const float& v1, const float& v2, double t)
	{
		return (1 - t) * v1 + t * v2;
	}

	static double lerp(const double& v1, const double& v2, double t)
	{
		return (1 - t) * v1 + t * v2;
	}

	static Vector3f lerp(const Vector3f& vec1, const Vector3f& vec2, double t)
	{
		Vector3f vec;
		vec.x = lerp(vec1.x, vec2.x, t);
		vec.y = lerp(vec1.y, vec2.y, t);
		vec.z = lerp(vec1.z, vec2.z, t);
		return vec;
	}

	static Vector3d lerp(const Vector3d& vec1, const Vector3d& vec2, double t)
	{
		Vector3d vec;
		vec.x = lerp(vec1.x, vec2.x, t);
		vec.y = lerp(vec1.y, vec2.y, t);
		vec.z = lerp(vec1.z, vec2.z, t);
		return vec;
	}

	// Split double into high and low floats
	static void split_double(const double& in, float& high, float& low)
	{
		high = static_cast<float>(in);
		low = static_cast<float>(in - static_cast<double>(high));
	}

	// Split each component of double vector into high and low parts
	static void split_double(const Vector3d& in, Vector3f& high, Vector3f& low)
	{
		split_double(in.x, high.x, low.x);
		split_double(in.y, high.y, low.y);
		split_double(in.z, high.z, low.z);
	}

	inline Quat rotation_between_vectors(Vector3f start, Vector3f target)
	{
		target = normalize(target);
		start = normalize(start);

		float cos_theta = dot(target, start);
		Vector3f rotation_axis;

		if (cos_theta < -1 + 0.001f)
		{
			rotation_axis = cross(Vector3f(0.0f, 0.0f, 1.0f), target);
			if (length_squared(rotation_axis) < 0.01) // bad luck, they were parallel, try again!
				rotation_axis = cross(Vector3f(1.0f, 0.0f, 0.0f), target);

			rotation_axis = normalize(rotation_axis);
			return glm::angleAxis(glm::radians(180.0f), static_cast<glm::vec3>(rotation_axis));
		}

		rotation_axis = cross(target, start);

		float s = sqrt((1 + cos_theta) * 2);
		float invs = 1 / s;

		return Quat{ rotation_axis.x * invs, rotation_axis.y * invs, rotation_axis.z * invs, s * 0.5f };
	}
}
