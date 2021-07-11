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

		Vector2(Float x_, Float y_)
		{
			x = x_;
			y = y_;
		}

		// Fill X/Y/Z Components with Val
		Vector2(Float val)
		{
			x = val;
			y = val;
		}

		Vector2(glm::vec2 vec)
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
		void operator=(glm::vec2 vec)
		{
			x = vec.x;
			y = vec.y;
		}

		// Operator+=
		void operator+=(glm::vec2 vec)
		{
			x += vec.x;
			y += vec.y;
		}

		void operator+=(Vector2 vec)
		{
			x += vec.x;
			y += vec.y;
		}

		// Operator-=
		void operator-=(glm::vec2 vec)
		{
			x -= vec.x;
			y -= vec.y;
		}

		void operator-=(Vector2 vec)
		{
			x -= vec.x;
			y -= vec.y;
		}

		// Operator+
		Vector2 operator+ (Vector2 vec) const
		{
			Vector2 vector;
			vector.x = x + vec.x;
			vector.y = y + vec.y;
			return vector;
		}

		Vector2 operator+ (glm::vec2 vec) const
		{
			Vector2 vector;
			vector = x + vec.x;
			vector = y + vec.y;
			return vector;
		}

		Vector2 operator+ (Float value) const
		{
			return Vector2(x + value, y + value);
		}

		// Operator-
		Vector2 operator- (Vector2 vec) const
		{
			Vector2 vector;
			vector.x = x - vec.x;
			vector.y = y - vec.y;
			return vector;
		}

		Vector2 operator- (glm::vec2 vec) const
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
		Vector2 operator* (Float inFloat) const
		{
			Vector2 vector;
			vector.x = x * inFloat;
			vector.y = y * inFloat;
			return vector;
		}

		// Operator*=
		void operator*= (Float inFloat)
		{
			x *= inFloat;
			y *= inFloat;
		}

		// Operator/
		Vector2 operator/ (Float inFloat) const
		{
			Vector2 vector;
			vector.x = x / inFloat;
			vector.y = y / inFloat;
			return vector;
		}
		
		// Operator/=
		void operator/= (Float inFloat)
		{
			x /= inFloat;
			y /= inFloat;
		}

		// Functions
		Float Dot(const Vector2& vec) const
		{
			return (x * vec.x) + (y * vec.y);
		}

		Float LengthSquared()
		{
			return x * x + y * y;
		}

		Float Length()
		{
			return sqrtf(LengthSquared());
		}

		Float DistanceToSquared(Vector2 vec)
		{
			Float deltaX = x - vec.x;
			Float deltaY = y - vec.y;

			return (deltaX * deltaX) + (deltaY * deltaY);
		}

		Float DistanceTo(Vector2 vec)
		{
			return sqrtf(DistanceTo(vec));
		}

		Vector2 Normalised()
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

		Vector3(Float x_, Float y_, Float z_)
		{
			x = x_;
			y = y_;
			z = z_;
		}

		// Fill X/Y/Z Components with Val
		Vector3(Float val)
		{
			x = val;
			y = val;
			z = val;
		}

		Vector3(glm::vec3 vec)
		{
			x = vec.x;
			y = vec.y;
			z = vec.z;
		}

		Vector3(Vector2 vec)
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
		void operator=(glm::vec3 vec)
		{
			x = vec.x;
			y = vec.y;
			z = vec.z;
		}

		void operator=(btVector3 vec)
		{
			x = vec.getX();
			y = vec.getY();
			z = vec.getZ();
		}

		// Operator+=
		void operator+=(glm::vec3 vec)
		{
			x += vec.x;
			y += vec.y;
			z += vec.z;
		}

		void operator+=(btVector3 vec)
		{
			x += vec.getX();
			y += vec.getY();
			z += vec.getZ();
		}

		void operator+=(Vector2 vec)
		{
			x += vec.x;
			y += vec.y;
		}

		// Operator-=
		void operator-=(glm::vec3 vec)
		{
			x -= vec.x;
			y -= vec.y;
			z -= vec.z;
		}

		void operator-=(btVector3 vec)
		{
			x -= vec.getX();
			y -= vec.getY();
			z -= vec.getZ();
		}

		// Operator+
		Vector3 operator+(Vector3 vec)
		{
			Vector3 vector;
			vector.x = x + vec.x;
			vector.y = y + vec.y;
			vector.z = z + vec.z;
			return vector;
		}

		Vector3 operator+ (glm::vec3 vec)
		{
			Vector3 vector;
			vector.x = x + vec.x;
			vector.y = y + vec.y;
			vector.z = z + vec.z;
			return vector;
		}

		Vector3 operator+(btVector3 vec)
		{
			Vector3 vector;
			vector.x = x + vec.getX();
			vector.y = y + vec.getY();
			vector.z = z + vec.getZ();
			return vector;
		}

		// Operator-
		Vector3 operator-(Vector3 vec)
		{
			Vector3 vector;
			vector.x = x - vec.x;
			vector.y = y - vec.y;
			vector.z = z - vec.z;
			return vector;
		}

		Vector3 operator- (glm::vec3 vec)
		{
			Vector3 vector;
			vector = x - vec.x;
			vector = y - vec.y;
			vector = z - vec.z;
			return vector;
		}

		Vector3 operator-(btVector3 vec)
		{
			Vector3 vector;
			vector.x = x - vec.getX();
			vector.y = y - vec.getY();
			vector.z = z - vec.getZ();
			return vector;
		}

		// Operator*
		Vector3 operator*(Vector3 vec)
		{
			Vector3 vector;
			vector.x = x * vec.x;
			vector.y = y * vec.y;
			vector.z = z * vec.z;
			return vector;
		}

		// Functions
		Float Dot(Vector3 vec)
		{
			return (x * vec.x) + (y * vec.y) + (z * vec.z);
		}

		Vector3 Cross(Vector3 vec)
		{
			Vector3 cross;
			cross.x = y * vec.z - z * vec.y;
			cross.y = x * vec.z - z * vec.x;
			cross.z = x * vec.y - y * vec.x;
			return cross;
		}

		Float LengthSquared()
		{
			return x * x + y * y + z * z;
		}

		Float Length()
		{
			return sqrtf(LengthSquared());
		}

		Vector3 Normalised()
		{
			Vector3 vector;
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