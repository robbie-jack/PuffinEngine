#pragma once

#ifndef MATHS_HELPERS_H
#define MATHS_HELPERS_H

#include <Types/Vector.h>

#include <algorithm>
#include <cmath>

#define PI 3.141592653589793238463

namespace Puffin
{
	namespace Maths
	{
		template<typename T>
		static inline T Clamp(const T& Value, const T& Min, const T& Max)
		{
			return std::max(Min, std::min(Max, Value));
		}

		static inline Vector2f Clamp(const Vector2f& Vector, const Vector2f& Min, const Vector2f& Max)
		{
			return Vector2f(Clamp(Vector.x, Min.x, Max.x), Clamp(Vector.y, Min.y, Max.y));
		}

		static inline double DegreesToRadians(double degrees)
		{
			return degrees * (PI / 180.0f);
		}

		static inline double RadiansToDegrees(double radians)
		{
			return radians * (180.0f / PI);
		}

		static inline Vector2f RotatePointAroundPoint(const Vector2f inPoint, const Vector2f rotatePoint, const double inAngle)
		{
			Vector2f newPoint;
			double angleRad = DegreesToRadians(inAngle);

			newPoint.x = ((inPoint.x - rotatePoint.x) * std::cos(angleRad)) - ((inPoint.y - rotatePoint.y) * std::sin(angleRad)) + rotatePoint.x;
			newPoint.y = ((inPoint.y - rotatePoint.y) * std::sin(angleRad)) + ((inPoint.y - rotatePoint.y) * std::cos(angleRad)) + rotatePoint.y;

			return newPoint;
		}

		// Rotate Point around Origin by Angle
		static inline Vector2f RotatePointAroundOrigin(const Vector2f& inPoint, const double inAngle)
		{
			return RotatePointAroundPoint(inPoint, Vector2f(0.0f), inAngle);
		}

		static inline float Lerp(const float& v1, const float& v2, double t)
		{
			return (1 - t) * v1 + t * v2;
		}

		static inline double Lerp(const double& v1, const double& v2, double t)
		{
			return (1 - t) * v1 + t * v2;
		}

		static inline Vector3f Lerp(const Vector3f& vec1, const Vector3f& vec2, double t)
		{
			Vector3f vec;
			vec.x = Lerp(vec1.x, vec2.x, t);
			vec.y = Lerp(vec1.y, vec2.y, t);
			vec.z = Lerp(vec1.z, vec2.z, t);
			return vec;
		}

		static inline Vector3d Lerp(const Vector3d& vec1, const Vector3d& vec2, double t)
		{
			Vector3d vec;
			vec.x = Lerp(vec1.x, vec2.x, t);
			vec.y = Lerp(vec1.y, vec2.y, t);
			vec.z = Lerp(vec1.z, vec2.z, t);
			return vec;
		}
	}
}

#endif //MATHS_HELPERS_H