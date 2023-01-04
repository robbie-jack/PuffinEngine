#pragma once

#ifndef QUAT_H
#define QUAT_H

#include <Types/Vector.h>
#include <Types/Matrix.h>

namespace Puffin
{
	namespace Maths
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

			Quat(float inX, float inY, float inZ, float inW) : x(inX), y(inY), z(inZ), w(inW) {}

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

			float w, x, y, z;
		};
	}
}

#endif // QUAT_H