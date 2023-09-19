#pragma once

#include "box2d/box2d.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

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
		T dot(const Vector2<T>& vec) const
		{
			return (x * vec.x) + (y * vec.y);
		}

		T cross(const Vector2<T>& vec) const
		{
			return x * vec.y - y * vec.x;
		}

		Vector2<T> perpendicularClockwise() const
		{
			return Vector2<T>(y, -x);
		}

		Vector2<T> perpendicularCounterClockwise() const
		{
			return Vector2<T>(-y, x);
		}

		T lengthSquared() const
		{
			return x * x + y * y;
		}

		T length() const
		{
			return sqrtf(lengthSquared());
		}

		T distanceToSquared(const Vector2& vec)
		{
			T deltaX = x - vec.x;
			T deltaY = y - vec.y;

			return (deltaX * deltaX) + (deltaY * deltaY);
		}

		T distanceTo(const Vector2& vec)
		{
			return sqrtf(DistanceTo(vec));
		}

		void normalize()
		{
			T lengthT = length();

			x /= lengthT;
			y /= lengthT;
		}

		Vector2 normalized() const
		{
			Vector2 vector = *this;
			T lengthT = length();

			vector.x /= lengthT;
			vector.y /= lengthT;

			return vector;
		}

		void zero()
		{
			x = 0.0f;
			y = 0.0f;
		}

		[[nodiscard]] Vector2 abs() const
		{
			return Vector2(std::abs(x), std::abs(y));
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
	template <typename T>
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

		Vector3(const Vector2<T>& vec)
		{
			x = vec.x;
			y = vec.y;
			z = 0.0f;
		}

		// Operator Overrides
		bool operator==(const Vector3<T>& vec) const
		{
			return x == vec.x && y == vec.y && z == vec.z;
		}

		// Operator Conversion
		explicit operator glm::vec3() const
		{
			const glm::vec3 vec = {x, y, z};
			return vec;
		}

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
		T operator[](const int idx) const
		{
			assert(idx >= 0 && idx < 3);

			return (&x)[idx];
		}

		T& operator[](const int idx)
		{
			assert(idx >= 0 && idx < 3);

			return (&x)[idx];
		}

		// Functions
		T dot(const Vector3& vec) const
		{
			T temp = (x * vec.x) + (y * vec.y) + (z * vec.z);
			return temp;
		}

		Vector3 cross(const Vector3& vec)
		{
			Vector3 cross;
			cross.x = y * vec.z - z * vec.y;
			cross.y = x * vec.z - z * vec.x;
			cross.z = x * vec.y - y * vec.x;
			return cross;
		}

		T lengthSquared() const
		{
			return x * x + y * y + z * z;
		}

		T length() const
		{
			return sqrtf(lengthSquared());
		}

		void normalize()
		{
			T lengthT = length();

			x /= lengthT;
			y /= lengthT;
			z /= lengthT;
		}

		[[nodiscard]] Vector3 normalized() const
		{
			Vector3 vector = *this;
			T lengthT = length();

			vector.x /= lengthT;
			vector.y /= lengthT;
			vector.z /= lengthT;

			return vector;
		}

		void zero()
		{
			x = 0.0f;
			y = 0.0f;
			z = 0.0f;
		}

		Vector2<T> xy() const
		{
			return Vector2<T>(x, y);
		}


		// Json
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Vector3, x, y, z)
	};

	using Vector3f = Vector3<float>;
	using Vector3d = Vector3<double>;
	using Vector3i = Vector3<int>;
}

namespace std
{
	template <>
	struct hash<puffin::Vector2f>
	{
		size_t operator()(const puffin::Vector2f& vec) const noexcept
		{
			return (hash<float>()(vec.x) ^
				(hash<float>()(vec.y) << 1) >> 1);
		}
	};

	template <>
	struct hash<puffin::Vector3f>
	{
		size_t operator()(const puffin::Vector3f& vec) const noexcept
		{
			return (hash<float>()(vec.x) ^
				(hash<float>()(vec.y) << 1) ^
				(hash<float>()(vec.z) << 1) >> 1);
		}
	};

	template <>
	struct hash<puffin::Vector3d>
	{
		size_t operator()(const puffin::Vector3d& vec) const noexcept
		{
			return (hash<double>()(vec.x) ^
				(hash<double>()(vec.y) << 1) ^
				(hash<double>()(vec.z) << 1) >> 1);
		}
	};
}
