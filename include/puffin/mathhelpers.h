#pragma once

#include <algorithm>
#include <cmath>

#include "puffin/types/vector.h"
#include "puffin/types/quat.h"

#define PI 3.141592653589793238463

namespace puffin::maths
{
	template<typename T>
	static T Clamp(const T& value, const T& min, const T& max)
	{
		return std::max(min, std::min(max, value));
	}

	static Vector2f Clamp(const Vector2f& vector, const Vector2f& min, const Vector2f& max)
	{
		return Vector2f(Clamp(vector.x, min.x, max.x), Clamp(vector.y, min.y, max.y));
	}

	static float DegToRad(const float degrees)
	{
		return degrees * (PI / 180.0);
	}

	static float RadToDeg(const float radians)
	{
		return radians * (180.0 / PI);
	}

	static double DegToRad(const double degrees)
	{
		return degrees * (PI / 180.0);
	}

	static double RadToDeg(const double radians)
	{
		return radians * (180.0 / PI);
	}

	static Vector3f DegToRad(const Vector3f& vecD)
	{
		Vector3f vecR;

		vecR.x = DegToRad(vecD.x);
		vecR.y = DegToRad(vecD.y);
		vecR.z = DegToRad(vecD.z);

		return vecR;
	}

	static Vector3f RadToDeg(const Vector3f& vecR)
	{
		Vector3f vecD;

		vecD.x = RadToDeg(vecR.x);
		vecD.y = RadToDeg(vecR.y);
		vecD.z = RadToDeg(vecR.z);

		return vecD;
	}

	static Vector2f RotatePointAroundPoint(const Vector2f inPoint, const Vector2f rotatePoint, const double inAngle)
	{
		Vector2f newPoint;
		const double angleRad = DegToRad(inAngle);

		newPoint.x = ((inPoint.x - rotatePoint.x) * std::cos(angleRad)) - ((inPoint.y - rotatePoint.y) * std::sin(angleRad)) + rotatePoint.x;
		newPoint.y = ((inPoint.y - rotatePoint.y) * std::sin(angleRad)) + ((inPoint.y - rotatePoint.y) * std::cos(angleRad)) + rotatePoint.y;

		return newPoint;
	}

	// Rotate Point around Origin by Angle
	static Vector2f RotatePointAroundOrigin(const Vector2f& inPoint, const double inAngle)
	{
		return RotatePointAroundPoint(inPoint, Vector2f(0.0f), inAngle);
	}

	static float Lerp(const float& v1, const float& v2, double t)
	{
		return (1 - t) * v1 + t * v2;
	}

	static double Lerp(const double& v1, const double& v2, double t)
	{
		return (1 - t) * v1 + t * v2;
	}

	static Vector3f Lerp(const Vector3f& vec1, const Vector3f& vec2, double t)
	{
		Vector3f vec;
		vec.x = Lerp(vec1.x, vec2.x, t);
		vec.y = Lerp(vec1.y, vec2.y, t);
		vec.z = Lerp(vec1.z, vec2.z, t);
		return vec;
	}

	static Vector3d Lerp(const Vector3d& vec1, const Vector3d& vec2, double t)
	{
		Vector3d vec;
		vec.x = Lerp(vec1.x, vec2.x, t);
		vec.y = Lerp(vec1.y, vec2.y, t);
		vec.z = Lerp(vec1.z, vec2.z, t);
		return vec;
	}

	// Split double into high and low floats
	static void SplitDouble(const double& in, float& high, float& low)
	{
		high = static_cast<float>(in);
		low = static_cast<float>(in - static_cast<double>(high));
	}

	// Split each component of double vector into high and low parts
	static void SplitDouble(const Vector3d& in, Vector3f& high, Vector3f& low)
	{
		SplitDouble(in.x, high.x, low.x);
		SplitDouble(in.y, high.y, low.y);
		SplitDouble(in.z, high.z, low.z);
	}

	inline Quat RotationBetweenVectors(Vector3f start, Vector3f target)
	{
		target = normalize(target);
		start = normalize(start);

		const float cosTheta = dot(target, start);
		Vector3f rotationAxis;

		if (cosTheta < -1 + 0.001f)
		{
			rotationAxis = cross(Vector3f(0.0f, 0.0f, 1.0f), target);
			if (length_squared(rotationAxis) < 0.01) // bad luck, they were parallel, try again!
				rotationAxis = cross(Vector3f(1.0f, 0.0f, 0.0f), target);

			rotationAxis = normalize(rotationAxis);
			return glm::angleAxis(glm::radians(180.0f), static_cast<glm::vec3>(rotationAxis));
		}

		rotationAxis = cross(target, start);

		float s = sqrt((1 + cosTheta) * 2);
		float invs = 1 / s;

		return Quat{ rotationAxis.x * invs, rotationAxis.y * invs, rotationAxis.z * invs, s * 0.5f };
	}
}
