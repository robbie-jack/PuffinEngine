#pragma once

#include "Types/Vector.h"
#include "Types/Matrix.h"
#include "MathHelpers.h"

#include "nlohmann/json.hpp"

namespace puffin::maths
{
	/*
	====================
	Quat
	====================
	*/
	struct Quat
	{
		Quat() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}

		Quat(const Quat& quat) : x(quat.x), y(quat.y), z(quat.z), w(quat.w) {}

		Quat(float inX, float inY, float inZ, float inW = 1.0f) : x(inX), y(inY), z(inZ), w(inW) {}

		Quat(Vector3f n, const float angleRadians)
		{
			const float halfAngleRadians = 0.5f * angleRadians;

			w = cosf(halfAngleRadians);

			const float halfSine = sinf(halfAngleRadians);
			const Vector3f nNormal = n.normalized();
			x = nNormal.x * halfSine;
			y = nNormal.y * halfSine;
			z = nNormal.z * halfSine;
		}

		const Quat& operator=(const Quat& quat)
		{
			x = quat.x;
			y = quat.y;
			z = quat.z;
			w = quat.w;
			return *this;
		}

		Quat& operator*= (const float& val)
		{
			x *= val;
			y *= val;
			z *= val;
			w *= val;
			return *this;
		}

		Quat& operator*= (const Quat& quat)
		{
			Quat temp = *this * quat;
			w = temp.w;
			x = temp.x;
			y = temp.y;
			z = temp.z;
			return *this;
		}

		Quat operator* (const Quat& quat) const
		{
			Quat temp;
			temp.w = (w * quat.w) - (x * quat.x) - (y * quat.y) - (z * quat.z);
			temp.x = (x * quat.w) + (w * quat.x) + (y * quat.y) - (z * quat.z);
			temp.y = (y * quat.w) + (w * quat.x) + (z * quat.y) - (x * quat.z);
			temp.z = (z * quat.w) + (w * quat.x) + (x * quat.y) - (y * quat.z);
			return temp;
		}

		void normalize()
		{
			if (const float invMag = 1.0f / magnitude(); invMag * 0.0f == invMag * 0.0f)
			{
				x = x * invMag;
				y = y * invMag;
				z = z * invMag;
				w = w * invMag;
			}
		}

		void invert()
		{
			*this *= 1.0f / magnitudeSq();
			x = -x;
			y = -y;
			z = -z;
		}

		Quat inverse() const
		{
			Quat val(*this);
			val.invert();
			return val;
		}

		float magnitudeSq() const
		{
			return x * x + y * y + z * z + w * w;
		}

		float magnitude() const
		{
			return sqrtf(magnitudeSq());
		}

		Vector3f rotatePoint(const Vector3f& vec) const
		{
			const Quat vector(vec.x, vec.y, vec.z, 0.0f);
			const Quat finalQuat = *this * vector * inverse();
			return Vector3f(finalQuat.x, finalQuat.y, finalQuat.z);
		}

		Mat3 rotateMatrix(const Mat3& rhs) const
		{
			Mat3 mat;
			mat.rows[0] = rotatePoint(rhs.rows[0]);
			mat.rows[1] = rotatePoint(rhs.rows[1]);
			mat.rows[2] = rotatePoint(rhs.rows[2]);
			return mat;
		}

		Vector3f xyz() const
		{
			return Vector3f(x, y, z);
		}

		bool isValid() const
		{
			if (x * 0 != x * 0)
				return false;

			if (y * 0 != y * 0)
				return false;

			if (z * 0 != z * 0)
				return false;

			if (w * 0 != w * 0)
				return false;

			return true;
		}

		Mat3 toMat3() const
		{
			Mat3 mat;
			mat.identity();

			mat.rows[0] = rotatePoint(mat.rows[0]);
			mat.rows[1] = rotatePoint(mat.rows[1]);
			mat.rows[2] = rotatePoint(mat.rows[2]);
			return mat;
		}

		// Generate quaternion from euler angles, Pitch (X), Yaw (Y), Roll (Z)
		static Quat fromEulerAngles(double pitch, double yaw, double roll)
		{
			const double cr = cos(roll * 0.5);
			const double cp = cos(pitch * 0.5);
			const double cy = cos(yaw * 0.5);
			
			const double sr = sin(roll * 0.5);
			const double sp = sin(pitch * 0.5);
			const double sy = sin(yaw * 0.5);

			const double cpcy = cp * cy;
			const double spsy = sp * sy;

			Quat q;

			q.w = cr * cpcy + sr * spsy;
			q.x = sr * cpcy - cr * spsy;
			q.y = cr * sp * cy + sr * cp * sy;
			q.z = cr * cp * sy - sr * sp * cy;

			return q;
		}

		Vector3f toEulerAngles() const
		{
			Vector3f angles;

			// roll (x-axis rotation)
			const double sinrCosp = 2 * (w * x + y * z);
			double cosrCosp = 1 - 2 * (x * x + y * y);
			angles.x = std::atan2(sinrCosp, cosrCosp);

			// pitch (y-axis rotation)
			const double sinp = std::sqrt(1 + 2 * (w * y - x * z));
			const double cosp = std::sqrt(1 - 2 * (w * y - x * z));
			angles.y = 2 * std::atan2(sinp, cosp) - PI / 2;

			// yaw (z-axis rotation)
			const double sinyCosp = 2 * (w * z + x * y);
			const double cosyCosp = 1 - 2 * (y * y + z * z);
			angles.z = std::atan2(sinyCosp, cosyCosp);

			return angles;
		}

		float x, y, z, w;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Quat, x, y, z, w)
	};
}