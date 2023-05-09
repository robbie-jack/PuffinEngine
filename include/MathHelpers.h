#pragma once

#include <Types/Vector.h>

#include <algorithm>
#include <cmath>

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

	static float degToRad(const float degrees)
	{
		return degrees * (PI / 180.0);
	}

	static float radToDeg(const float radians)
	{
		return radians * (180.0 / PI);
	}

	static double degToRad(const double degrees)
	{
		return degrees * (PI / 180.0);
	}

	static double radToDeg(const double radians)
	{
		return radians * (180.0 / PI);
	}

	static Vector3f radToDeg(const Vector3f& vecR)
	{
		Vector3f vecD;

		vecD.x = radToDeg(vecR.x);
		vecD.y = radToDeg(vecR.y);
		vecD.z = radToDeg(vecR.z);

		return vecD;
	}

	static Vector2f rotatePointAroundPoint(const Vector2f inPoint, const Vector2f rotatePoint, const double inAngle)
	{
		Vector2f newPoint;
		const double angleRad = degToRad(inAngle);

		newPoint.x = ((inPoint.x - rotatePoint.x) * std::cos(angleRad)) - ((inPoint.y - rotatePoint.y) * std::sin(angleRad)) + rotatePoint.x;
		newPoint.y = ((inPoint.y - rotatePoint.y) * std::sin(angleRad)) + ((inPoint.y - rotatePoint.y) * std::cos(angleRad)) + rotatePoint.y;

		return newPoint;
	}

	// Rotate Point around Origin by Angle
	static Vector2f rotatePointAroundOrigin(const Vector2f& inPoint, const double inAngle)
	{
		return rotatePointAroundPoint(inPoint, Vector2f(0.0f), inAngle);
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
	static void splitDouble(const double& in, float& high, float& low)
	{
		high = static_cast<float>(in);
		low = static_cast<float>(in - static_cast<double>(high));
	}

	// Split each component of double vector into high and low parts
	static void splitDouble(const Vector3d& in, Vector3f& high, Vector3f& low)
	{
		splitDouble(in.x, high.x, low.x);
		splitDouble(in.y, high.y, low.y);
		splitDouble(in.z, high.z, low.z);
	}
}
