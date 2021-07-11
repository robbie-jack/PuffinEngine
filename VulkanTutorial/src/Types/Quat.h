//#pragma once
//
//#ifndef QUAT_H
//#define QUAT_H
//
//#include <Types/Vector.h>
//
//namespace Puffin
//{
//	namespace Maths
//	{
//		/*
//		====================
//		Quat
//		====================
//		*/
//		struct Quat
//		{
//			Quat() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
//
//			Quat(const Quat& quat) : x(quat.x), y(quat.y), z(quat.z), w(quat.w) {}
//
//			Quat(Float inX, Float inY, Float inZ, Float inW) : x(inX), y(inY), z(inZ), w(inW) {}
//
//			Quat(Vector3 n, const Float angleRadians)
//			{
//				const Float halfAngleRadians = 0.5f * angleRadians;
//
//				w = cosf(halfAngleRadians);
//
//				const Float halfSine = sinf(halfAngleRadians);
//				Vector3 nNormal = n.Normalised();
//				x = nNormal.x * halfSine;
//				y = nNormal.y * halfSine;
//				z = nNormal.z * halfSine;
//			}
//
//			const Quat& operator=(const Quat& quat)
//			{
//				x = quat.x;
//				y = quat.y;
//				z = quat.z;
//				w = quat.w;
//				return *this;
//			}
//
//			Quat& operator*= (const Float& val)
//			{
//				x *= val;
//				y *= val;
//				z *= val;
//				w *= val;
//				return *this;
//			}
//
//			Quat& operator*= (const Quat* quat)
//			{
//				Quat temp = *this * quat;
//				w = temp.w;
//				x = temp.x;
//				y = temp.y;
//				z = temp.z;
//				return *this;
//			}
//
//			Quat operator* (const Quat& quat) const
//			{
//				Quat temp;
//				temp.w = (w * quat.w) - (x * quat.x) - (y * quat.y) - (z * quat.z);
//				temp.x = (x * quat.w) + (w * quat.x) + (y * quat.y) - (z * quat.z);
//				temp.y = (y * quat.w) + (w * quat.x) + (z * quat.y) - (x * quat.z);
//				temp.z = (z * quat.w) + (w * quat.x) + (x * quat.y) - (y * quat.z);
//				return temp;
//			}
//
//			void Normalize()
//			{
//				Float invMag = 1.0f / GetMagnitude();
//
//				if (invMag * 0.0f == invMag * 0.0f)
//				{
//					x = x * invMag;
//					y = y * invMag;
//					z = z * invMag;
//					w = w * invMag;
//				}
//			}
//
//			void Invert()
//			{
//				*this *= 1.0f / MagnitudeSquared();
//				x = -x;
//				y = -y;
//				z = -z;
//			}
//
//			Quat Inverse() const
//			{
//				Quat val(*this);
//				val.Invert();
//				return val;
//			}
//
//			Float MagnitudeSquared() const
//			{
//				return x * x + y * y + z * z + w * w;
//			}
//
//			Float GetMagnitude() const
//			{
//				return sqrtf(MagnitudeSquared());
//			}
//
//			Vector3 RotatePoint(const Vector3& vec)
//			{
//				Quat vector(vec.x, vec.y, vec.z, 0.0f);
//				Quat final = *this * vector * Inverse();
//				return Vector3(final.x, final.y, final.z);
//			}
//
//			Mat3 RotateMatrix(const Mat3& mat)
//			{
//				Mat3 mat;
//				mat.rows[0] = RotatePoint(mat.rows[0]);
//				mat.rows[1] = RotatePoint(mat.rows[1]);
//				mat.rows[2] = RotatePoint(mat.rows[2]);
//				return mat;
//			}
//
//			Vector3 GetXYZ() const
//			{
//				return Vector3(x, y, z);
//			}
//
//			bool IsValid() const
//			{
//				if (x * 0 != x * 0)
//					return false;
//
//				if (y * 0 != y * 0)
//					return false;
//
//				if (z * 0 != z * 0)
//					return false;
//
//				if (w * 0 != w * 0)
//					return false;
//
//				return true;
//			}
//
//			Mat3 ToMat3() const
//			{
//				Mat3 mat;
//				mat.Identity();
//
//				mat.rows[0] = RotatePoint(mat.rows[0]);
//				mat.rows[1] = RotatePoint(mat.rows[1]);
//				mat.rows[2] = RotatePoint(mat.rows[2]);
//				return mat;
//			}
//
//			Float w, x, y, z;
//		};
//	}
//}
//
//#endif // QUAT_H