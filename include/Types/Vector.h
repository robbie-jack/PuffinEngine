#pragma once

#include "box2d/box2d.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
//#include "bx/math.h"

#include "nlohmann/json.hpp"

#include <cmath>

using json = nlohmann::json;

namespace puffin
{
	/*
	====================
	Vector 2
	====================
	*/
	template<typename T>
	struct Vector2
	{
		T x, y;

		// Constructors
		Vector2()
		{
			x = 0;
			y = 0;
		}

		Vector2(const T& x_, const T& y_)
		{
			x = x_;
			y = y_;
		}

		// Fill X/Y/Z Components with Val
		Vector2(const T& val)
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
		bool operator== (const Vector2<T>& vec)
		{
			return x == vec.x && y == vec.y;
		}

		// Operator Conversion
		explicit operator glm::vec2() const
		{
			glm::vec2 vec;
			vec.x = x;
			vec.y = y;
			return vec;
		}

		operator b2Vec2() const
		{
			b2Vec2 vec;
			vec.x = x;
			vec.y = y;
			return vec;
		}

		operator Vector2<float>() const
		{
			return Vector2<float>(x, y);
		}

		operator Vector2<double>() const
		{
			return Vector2<double>(x, y);
		}

		// Operator=
		void operator=(const T* rhs)
		{
			x = rhs[0];
			y = rhs[1];
		}

		// Operator+=
		void operator+=(const Vector2& vec)
		{
			x += vec.x;
			y += vec.y;
		}

		// Operator-=
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

		Vector2 operator+ (const T& value) const
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

		Vector2 operator-() const
		{
			Vector2 vector;
			vector.x = -x;
			vector.y = -y;
			return vector;
		}

		// Operator*
		Vector2 operator* (const T& inFloat) const
		{
			Vector2 vector;
			vector.x = x * inFloat;
			vector.y = y * inFloat;
			return vector;
		}

		Vector2 operator* (const Vector2& inVec) const
		{
			Vector2 vector;
			vector.x = x * inVec.x;
			vector.y = y * inVec.y;
			return vector;
		}

		// Operator*=
		void operator*= (const T& inFloat)
		{
			x *= inFloat;
			y *= inFloat;
		}

		// Operator/
		Vector2 operator/ (const T& inFloat) const
		{
			Vector2 vector;
			vector.x = x / inFloat;
			vector.y = y / inFloat;
			return vector;
		}
		
		// Operator/=
		void operator/= (const T& inFloat)
		{
			x /= inFloat;
			y /= inFloat;
		}

		// Operator[]
		T operator[] (const int idx) const
		{
			assert(idx >= 0 && idx < 2);

			return (&x)[idx];
		}

		T& operator[] (const int idx)
		{
			assert(idx >= 0 && idx < 2);

			return (&x)[idx];
		}

		// Functions
		T Dot(const Vector2<T>& vec) const
		{
			return (x * vec.x) + (y * vec.y);
		}

		T Cross(const Vector2<T>& vec) const
		{
			return x * vec.y - y * vec.x;
		}

		Vector2<T> PerpendicularClockwise() const
		{
			return Vector2<T>(y, -x);
		}

		Vector2<T> PerpendicularCounterClockwise() const
		{
			return Vector2<T>(-y, x);
		}

		T LengthSquared() const
		{
			return x * x + y * y;
		}

		T Length() const
		{
			return sqrtf(LengthSquared());
		}

		T DistanceToSquared(const Vector2& vec)
		{
			T deltaX = x - vec.x;
			T deltaY = y - vec.y;

			return (deltaX * deltaX) + (deltaY * deltaY);
		}

		T DistanceTo(const Vector2& vec)
		{
			return sqrtf(DistanceTo(vec));
		}

		void Normalise()
		{
			T length = Length();

			x /= length;
			y /= length;
		}

		Vector2 Normalised() const
		{
			Vector2 vector = *this;
			T length = Length();

			vector.x /= length;
			vector.y /= length;

			return vector;
		}

		void Zero()
		{
			x = 0.0f;
			y = 0.0f;
		}

		Vector2 Abs() const
		{
			return Vector2(abs(x), abs(y));
		}

		// Json
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Vector2, x, y)
	};

	typedef Vector2<float> Vector2f;
	typedef Vector2<double> Vector2d;
	typedef Vector2<int> Vector2i;

	/*
	====================
	Vector 3
	====================
	*/
	template<typename T>
	struct Vector3
	{
		T x, y, z;

		// Constructors
		Vector3()
		{
			x = 0;
			y = 0;
			z = 0;
		}

		Vector3(const T& x_, const T& y_, const T& z_)
		{
			x = x_;
			y = y_;
			z = z_;
		}

		// Fill X/Y/Z Components with Val
		Vector3(const T& val)
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

		/*Vector3(const bx::Vec3& vec)
		{
			x = vec.x;
			y = vec.y;
			z = vec.z;
		}*/

		Vector3(const Vector2<T>& vec)
		{
			x = vec.x;
			y = vec.y;
			z = 0.0f;
		}

		// Operator Overrides
		bool operator== (const Vector3<T>& vec) const
		{
			return x == vec.x && y == vec.y && z == vec.z;
		}

		// Operator Conversion
		explicit operator glm::vec3() const
		{
			const glm::vec3 vec = {x, y, z};
			return vec;
		}

		/*explicit operator bx::Vec3() const
		{
			const bx::Vec3 vec = 
			{
				static_cast<float>(x),
				static_cast<float>(y),
				static_cast<float>(z)
			};
			return vec;
		}*/

		operator Vector3<float>() const
		{
			return Vector3<float>(x, y, z);
		}

		operator Vector3<double>() const
		{
			return Vector3<double>(x, y, z);
		}

		// Operator=
		Vector3<T>& operator=(const Vector3& vec)
		{
			x = vec.x;
			y = vec.y;
			z = vec.z;
			return *this;
		}

		Vector3<T>& operator=(const T* rhs)
		{
			x = rhs[0];
			y = rhs[1];
			z = rhs[2];
			return *this;
		}

		// Operator+=
		void operator+=(const Vector3& vec)
		{
			x += vec.x;
			y += vec.y;
			z += vec.z;
		}

		void operator+=(const Vector2<T>& vec)
		{
			x += vec.x;
			y += vec.y;
		}

		// Operator-=
		void operator-=(const Vector3& vec)
		{
			x -= vec.x;
			y -= vec.y;
			z -= vec.z;
		}

		void operator-=(const Vector2<T>& vec)
		{
			x -= vec.x;
			y -= vec.y;
		}

		// Operator+
		Vector3 operator+(const Vector3& vec) const
		{
			Vector3 vector;
			vector.x = x + vec.x;
			vector.y = y + vec.y;
			vector.z = z + vec.z;
			return vector;
		}

		Vector3 operator+(const Vector2<T>& vec) const
		{
			Vector3 vector;
			vector.x = x + vec.x;
			vector.y = y + vec.y;
			return vector;
		}

		// Operator-
		Vector3 operator-(const Vector3& vec) const
		{
			Vector3 vector;
			vector.x = x - vec.x;
			vector.y = y - vec.y;
			vector.z = z - vec.z;
			return vector;
		}

		// Operator*
		Vector3 operator*(const Vector3& vec) const
		{
			Vector3 vector;
			vector.x = x * vec.x;
			vector.y = y * vec.y;
			vector.z = z * vec.z;
			return vector;
		}

		Vector3 operator*(const T& rhs) const
		{
			Vector3 vector;
			vector.x = x * rhs;
			vector.y = y * rhs;
			vector.z = z * rhs;
			return vector;
		}

		// Operator*=
		const Vector3& operator*=(const T& rhs)
		{
			x *= rhs;
			y *= rhs;
			z *= rhs;
			return *this;
		}

		// Operator[]
		T operator[] (const int idx) const
		{
			assert(idx >= 0 && idx < 3);

			return (&x)[idx];
		}

		T& operator[] (const int idx)
		{
			assert(idx >= 0 && idx < 3);

			return (&x)[idx];
		}

		// Functions
		T Dot(const Vector3& vec) const
		{
			T temp = (x * vec.x) + (y * vec.y) + (z * vec.z);
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

		T LengthSquared() const
		{
			return x * x + y * y + z * z;
		}

		T Length() const
		{
			return sqrtf(LengthSquared());
		}

		void Normalise()
		{
			T length = Length();

			x /= length;
			y /= length;
			z /= length;
		}

		Vector3 Normalised() const
		{
			Vector3 vector = *this;
			T length = Length();

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

		Vector2<T> GetXY() const
		{
			return Vector2<T>(x, y);
		}


		// Json
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Vector3, x, y, z)
	};

	typedef Vector3<float> Vector3f;
	typedef Vector3<double> Vector3d;
	typedef Vector3<int> Vector3i;
}

namespace std
{
	template<> struct hash<puffin::Vector2f>
	{
		size_t operator()(puffin::Vector2f const& vec) const
		{
			return (hash<float>()(vec.x) ^
				(hash<float>()(vec.y) << 1) >> 1);
		}
	};

	template<> struct hash<puffin::Vector3f>
	{
		size_t operator()(puffin::Vector3f const& vec) const
		{
			return (hash<float>()(vec.x) ^
				(hash<float>()(vec.y) << 1) ^
				(hash<float>()(vec.z) << 1) >> 1);
		}
	};

	template<> struct hash<puffin::Vector3d>
	{
		size_t operator()(puffin::Vector3d const& vec) const
		{
			return (hash<double>()(vec.x) ^
				(hash<double>()(vec.y) << 1) ^
				(hash<double>()(vec.z) << 1) >> 1);
		}
	};
}