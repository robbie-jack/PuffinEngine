#ifndef VECTOR_H
#define VECTOR_H

#include <btBulletDynamicsCommon.h>

#include <math.h>
#include <glm/gtc/matrix_transform.hpp>

//#define PFN_USE_DOUBLE_PRECISION

namespace Puffin
{
	#ifdef PFN_USE_DOUBLE_PRECISION
		typedef double Float;
	#else
		typedef float Float;
	#endif

	/*
	====================
	Vector 2
	====================
	*/
	struct Vector2
	{
		Float x, y;

		// Constructors
		Vector2()
		{
			x = 0.0f;
			y = 0.0f;
		}

		Vector2(const Float& x_, const Float& y_)
		{
			x = x_;
			y = y_;
		}

		// Fill X/Y/Z Components with Val
		Vector2(const Float& val)
		{
			x = val;
			y = val;
		}

		Vector2(const glm::vec2& vec)
		{
			x = vec.x;
			y = vec.y;
		}

		// Operator Overrides

		// Operator Conversion
		operator glm::vec2() const
		{
			glm::vec2 vec;
			vec.x = x;
			vec.y = y;
			return vec;
		}

		// Operator=
		void operator=(const glm::vec2& vec)
		{
			x = vec.x;
			y = vec.y;
		}

		void operator=(const Float* rhs)
		{
			x = rhs[0];
			y = rhs[1];
		}

		// Operator+=
		void operator+=(const glm::vec2& vec)
		{
			x += vec.x;
			y += vec.y;
		}

		void operator+=(const Vector2& vec)
		{
			x += vec.x;
			y += vec.y;
		}

		// Operator-=
		void operator-=(const glm::vec2& vec)
		{
			x -= vec.x;
			y -= vec.y;
		}

		void operator-=(const Vector2& vec)
		{
			x -= vec.x;
			y -= vec.y;
		}

		// Operator+
		Vector2 operator+ (const Vector2& vec) const
		{
			Vector2 vector;
			vector.x = x + vec.x;
			vector.y = y + vec.y;
			return vector;
		}

		Vector2 operator+ (const glm::vec2& vec) const
		{
			Vector2 vector;
			vector = x + vec.x;
			vector = y + vec.y;
			return vector;
		}

		Vector2 operator+ (const Float& value) const
		{
			return Vector2(x + value, y + value);
		}

		// Operator-
		Vector2 operator- (const Vector2& vec) const
		{
			Vector2 vector;
			vector.x = x - vec.x;
			vector.y = y - vec.y;
			return vector;
		}

		Vector2 operator- (const glm::vec2& vec) const
		{
			Vector2 vector;
			vector.x = x - vec.x;
			vector.y = y - vec.y;
			return vector;
		}

		Vector2 operator-() const
		{
			Vector2 vector;
			vector.x = -x;
			vector.y = y;
			return vector;
		}

		// Operator*
		Vector2 operator* (const Float& inFloat) const
		{
			Vector2 vector;
			vector.x = x * inFloat;
			vector.y = y * inFloat;
			return vector;
		}

		// Operator*=
		void operator*= (const Float& inFloat)
		{
			x *= inFloat;
			y *= inFloat;
		}

		// Operator/
		Vector2 operator/ (const Float& inFloat) const
		{
			Vector2 vector;
			vector.x = x / inFloat;
			vector.y = y / inFloat;
			return vector;
		}
		
		// Operator/=
		void operator/= (const Float& inFloat)
		{
			x /= inFloat;
			y /= inFloat;
		}

		// Operator[]
		Float operator[] (const int idx) const
		{
			assert(idx >= 0 && idx < 2);

			return (&x)[idx];
		}

		Float& operator[] (const int idx)
		{
			assert(idx >= 0 && idx < 2);

			return (&x)[idx];
		}

		// Functions
		Float Dot(const Vector2& vec) const
		{
			return (x * vec.x) + (y * vec.y);
		}

		Float Cross(const Vector2& vec) const
		{
			return x * vec.y - y * vec.x;
		}

		Float LengthSquared() const
		{
			return x * x + y * y;
		}

		Float Length() const
		{
			return sqrtf(LengthSquared());
		}

		Float DistanceToSquared(const Vector2& vec)
		{
			Float deltaX = x - vec.x;
			Float deltaY = y - vec.y;

			return (deltaX * deltaX) + (deltaY * deltaY);
		}

		Float DistanceTo(const Vector2& vec)
		{
			return sqrtf(DistanceTo(vec));
		}

		void Normalise()
		{
			Float length = Length();

			x /= length;
			y /= length;
		}

		Vector2 Normalised() const
		{
			Vector2 vector = *this;
			Float length = Length();

			vector.x /= length;
			vector.y /= length;

			return vector;
		}

		void Zero()
		{
			x = 0.0f;
			y = 0.0f;
		}

		// Serialization
		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(x, y);
		}
	};

	/*
	====================
	Vector 3
	====================
	*/
	struct Vector3
	{
		Float x, y, z;

		// Constructors
		Vector3()
		{
			x = 0.0f;
			y = 0.0f;
			z = 0.0f;
		}

		Vector3(const Float& x_, const Float& y_, const Float& z_)
		{
			x = x_;
			y = y_;
			z = z_;
		}

		// Fill X/Y/Z Components with Val
		Vector3(const Float& val)
		{
			x = val;
			y = val;
			z = val;
		}

		Vector3(const glm::vec3& vec)
		{
			x = vec.x;
			y = vec.y;
			z = vec.z;
		}

		Vector3(const Vector2& vec)
		{
			x = vec.x;
			y = vec.y;
			z = 0.0f;
		}

		// Operator Overrides

		// Operator Conversion
		operator glm::vec3() const
		{
			glm::vec3 vec;
			vec.x = x;
			vec.y = y;
			vec.z = z;
			return vec;
		}

		operator btVector3() const
		{
			btVector3 vec;
			vec.setValue(x, y, z);
			return vec;
		}

		// Operator=
		void operator=(const Vector3& vec)
		{
			x = vec.x;
			y = vec.y;
			z = vec.z;
		}

		void operator=(const glm::vec3& vec)
		{
			x = vec.x;
			y = vec.y;
			z = vec.z;
		}

		void operator=(const btVector3& vec)
		{
			x = vec.getX();
			y = vec.getY();
			z = vec.getZ();
		}

		void operator=(const Float* rhs)
		{
			x = rhs[0];
			y = rhs[1];
			z = rhs[2];
		}

		// Operator+=
		void operator+=(const glm::vec3& vec)
		{
			x += vec.x;
			y += vec.y;
			z += vec.z;
		}

		void operator+=(const btVector3& vec)
		{
			x += vec.getX();
			y += vec.getY();
			z += vec.getZ();
		}

		void operator+=(const Vector3& vec)
		{
			x += vec.x;
			y += vec.y;
			z += vec.z;
		}

		// Operator-=
		void operator-=(const glm::vec3& vec)
		{
			x -= vec.x;
			y -= vec.y;
			z -= vec.z;
		}

		void operator-=(const btVector3& vec)
		{
			x -= vec.getX();
			y -= vec.getY();
			z -= vec.getZ();
		}

		// Operator+
		Vector3 operator+(const Vector3& vec)
		{
			Vector3 vector;
			vector.x = x + vec.x;
			vector.y = y + vec.y;
			vector.z = z + vec.z;
			return vector;
		}

		Vector3 operator+ (const glm::vec3& vec)
		{
			Vector3 vector;
			vector.x = x + vec.x;
			vector.y = y + vec.y;
			vector.z = z + vec.z;
			return vector;
		}

		Vector3 operator+(const btVector3& vec)
		{
			Vector3 vector;
			vector.x = x + vec.getX();
			vector.y = y + vec.getY();
			vector.z = z + vec.getZ();
			return vector;
		}

		// Operator-
		Vector3 operator-(const Vector3& vec)
		{
			Vector3 vector;
			vector.x = x - vec.x;
			vector.y = y - vec.y;
			vector.z = z - vec.z;
			return vector;
		}

		Vector3 operator- (const glm::vec3& vec)
		{
			Vector3 vector;
			vector = x - vec.x;
			vector = y - vec.y;
			vector = z - vec.z;
			return vector;
		}

		Vector3 operator-(const btVector3& vec)
		{
			Vector3 vector;
			vector.x = x - vec.getX();
			vector.y = y - vec.getY();
			vector.z = z - vec.getZ();
			return vector;
		}

		// Operator*
		Vector3 operator*(const Vector3& vec)
		{
			Vector3 vector;
			vector.x = x * vec.x;
			vector.y = y * vec.y;
			vector.z = z * vec.z;
			return vector;
		}

		// Operator*=
		const Vector3& operator*=(const Float& rhs)
		{
			x *= rhs;
			y *= rhs;
			z *= rhs;
			return *this;
		}

		// Operator[]
		Float operator[] (const int idx) const
		{
			assert(idx >= 0 && idx < 3);

			return (&x)[idx];
		}

		Float& operator[] (const int idx)
		{
			assert(idx >= 0 && idx < 3);

			return (&x)[idx];
		}

		// Functions
		Float Dot(const Vector3& vec) const
		{
			Float temp = (x * vec.x) + (y * vec.y) + (z * vec.z);
			return temp;
		}

		Vector3 Cross(const Vector3& vec)
		{
			Vector3 cross;
			cross.x = y * vec.z - z * vec.y;
			cross.y = x * vec.z - z * vec.x;
			cross.z = x * vec.y - y * vec.x;
			return cross;
		}

		Float LengthSquared() const
		{
			return x * x + y * y + z * z;
		}

		Float Length() const
		{
			return sqrtf(LengthSquared());
		}

		void Normalise()
		{
			Float length = Length();

			x /= length;
			y /= length;
			z /= length;
		}

		Vector3 Normalised() const
		{
			Vector3 vector = *this;
			Float length = Length();

			vector.x /= length;
			vector.y /= length;
			vector.z /= length;

			return vector;
		}

		void Zero()
		{
			x = 0.0f;
			y = 0.0f;
			z = 0.0f;
		}

		Vector2 GetXY() const
		{
			return Vector2(x, y);
		}

		// Serialization
		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(x, y, z);
		}
	};
}

#endif // !VECTOR_H