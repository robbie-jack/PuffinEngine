#pragma once

#include "Types/Vector.h"
#include "Types/Matrix.h"
#include "MathHelpers.h"

#include "nlohmann/json.hpp"

namespace Puffin::Maths
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
			Vector3f nNormal = n.Normalised();
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

		void Normalize()
		{
			float invMag = 1.0f / GetMagnitude();

			if (invMag * 0.0f == invMag * 0.0f)
			{
				x = x * invMag;
				y = y * invMag;
				z = z * invMag;
				w = w * invMag;
			}
		}

		void Invert()
		{
			*this *= 1.0f / MagnitudeSquared();
			x = -x;
			y = -y;
			z = -z;
		}

		Quat Inverse() const
		{
			Quat val(*this);
			val.Invert();
			return val;
		}

		float MagnitudeSquared() const
		{
			return x * x + y * y + z * z + w * w;
		}

		float GetMagnitude() const
		{
			return sqrtf(MagnitudeSquared());
		}

		Vector3f RotatePoint(const Vector3f& vec) const
		{
			Quat vector(vec.x, vec.y, vec.z, 0.0f);
			Quat finalQuat = *this * vector * Inverse();
			return Vector3f(finalQuat.x, finalQuat.y, finalQuat.z);
		}

		Mat3 RotateMatrix(const Mat3& rhs)
		{
			Mat3 mat;
			mat.rows[0] = RotatePoint(rhs.rows[0]);
			mat.rows[1] = RotatePoint(rhs.rows[1]);
			mat.rows[2] = RotatePoint(rhs.rows[2]);
			return mat;
		}

		Vector3f GetXYZ() const
		{
			return Vector3f(x, y, z);
		}

		bool IsValid() const
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

		Mat3 ToMat3() const
		{
			Mat3 mat;
			mat.Identity();

			mat.rows[0] = RotatePoint(mat.rows[0]);
			mat.rows[1] = RotatePoint(mat.rows[1]);
			mat.rows[2] = RotatePoint(mat.rows[2]);
			return mat;
		}

		// Generate quaternion from euler angles, Pitch (X), Yaw (Y), Roll (Z)
		static Quat FromEulerAngles(double pitch, double yaw, double roll)
		{
			double cr = cos(roll * 0.5);
			double sr = sin(roll * 0.5);
			double cp = cos(pitch * 0.5);
			double sp = sin(pitch * 0.5);
			double cy = cos(yaw * 0.5);
			double sy = sin(yaw * 0.5);

			Quat q;

			q.w = cr * cp * cy + sr * sp * sy;
			q.x = sr * cp * cy - cr * sp * sy;
			q.y = cr * sp * cy + sr * cp * sy;
			q.z = cr * cp * sy - sr * sp * cy;

			return q;
		}

		// Generate euler angles, Pitch (X), Yaw (Y), Roll (Z) from quaternion in radians
		[[nodiscard]] Vector3f EulerAnglesRad() const
		{
			Vector3f angles;

			// roll (z-axis rotation)
			double sinr_cosp = 2 * (w * x + y * z);
			double cosr_cosp = 1 - 2 * (x * x + y * y);
			angles.z = std::atan2(sinr_cosp, cosr_cosp);

			// pitch (x-axis rotation)
			double sinp = std::sqrt(1 + 2 * (w * y - x * z));
			double cosp = std::sqrt(1 - 2 * (w * y - x * z));
			angles.x = 2 * std::atan2(sinp, cosp) - PI / 2;

			// yaw (y-axis rotation)
			double siny_cosp = 2 * (w * z + x * y);
			double cosy_cosp = 1 - 2 * (y * y + z * z);
			angles.y = std::atan2(siny_cosp, cosy_cosp);

			return angles;
		}

		// Generate euler angles, Pitch (X), Yaw (Y), Roll (Z) from quaternion in degrees
		[[nodiscard]] Vector3f EulerAnglesDeg() const
		{
			Vector3f angles = EulerAnglesRad();

			angles.x = Maths::RadiansToDegrees(angles.x);
			angles.y = Maths::RadiansToDegrees(angles.x);
			angles.z = Maths::RadiansToDegrees(angles.x);

			return angles;
		}

		float w, x, y, z;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Quat, w, x, y, z)
	};
}