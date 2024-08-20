#pragma once

#if PFN_BOX2D_PHYSICS
#include "box2d/box2d.h"
#endif

#include <cmath>

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

#include "nlohmann/json.hpp"

#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"

using json = nlohmann::json;

namespace puffin
{
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

		Vector3(const T& x, const T& y, const T& z)
			: x(x), y(y), z(z)
		{
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

		// Operator Conversion
		explicit operator glm::vec3() const
		{
			return glm::vec3(x, y, z);
		}

		explicit operator glm::dvec3() const
		{
			return glm::dvec3(x, y, z);
		}

		explicit operator Vector3<float>() const
		{
			return Vector3<float>(x, y, z);
		}

		explicit operator Vector3<double>() const
		{
			return Vector3<double>(x, y, z);
		}

		// Operator==
		bool operator==(const Vector3<T>& vec) const
		{
			return x == vec.x && y == vec.y && z == vec.z;
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

		// Operator-=
		void operator-=(const Vector3& vec)
		{
			x -= vec.x;
			y -= vec.y;
			z -= vec.z;
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

		const Vector3& operator*=(const Vector3& vec)
		{
			x *= vec.x;
			y *= vec.y;
			z *= vec.z;
			return *this;
		}

		// Operator/=
		Vector3& operator/=(const T& rhs)
		{
			x /= rhs;
			y /= rhs;
			z /= rhs;
			return *this;
		}

		// Operator[]
		T operator[](const int idx) const
		{
			assert(idx >= 0 && idx < 3);

			if (idx == 0)
				return x;

			if (idx == 1)
				return y;

			if (idx == 2)
				return z;

			return 0.0;
		}

		T& operator[](const int idx)
		{
			assert(idx >= 0 && idx < 3);

			if (idx == 0)
				return x;

			if (idx == 1)
				return y;

			if (idx == 2)
				return z;

			return 0.0;
		}

		void zero()
		{
			x = 0.0f;
			y = 0.0f;
			z = 0.0f;
		}

		// Json
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Vector3, x, y, z)
	};

	using Vector3f = Vector3<float>;
	using Vector3d = Vector3<double>;
	using Vector3i = Vector3<int>;

	inline float dot(const Vector3f& a, const Vector3f& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	inline double dot(const Vector3d& a, const Vector3d& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	inline Vector3f cross(const Vector3f& a, const Vector3f& b)
	{
		Vector3f cross;
		cross.x = a.y * b.z - a.z * b.y;
		cross.y = a.x * b.z - a.z * b.x;
		cross.z = a.x * b.y - a.y * b.x;
		return cross;
	}

	inline Vector3d cross(const Vector3d& a, const Vector3d& b)
	{
		Vector3d cross;
		cross.x = a.y * b.z - a.z * b.y;
		cross.y = a.x * b.z - a.z * b.x;
		cross.z = a.x * b.y - a.y * b.x;
		return cross;
	}

	inline float length_squared(const Vector3f& v)
	{
		return v.x * v.x + v.y * v.y + v.z * v.z;
	}

	inline float length(const Vector3f& v)
	{
		return sqrtf(length_squared(v));
	}

	inline double length_squared(const Vector3d& v)
	{
		return v.x * v.x + v.y * v.y + v.z * v.z;
	}

	inline double length(const Vector3d& v)
	{
		return sqrt(length_squared(v));
	}

	inline Vector3f normalize(const Vector3f& v)
	{
		Vector3f vec = v;
		const float l = length(v);

		vec /= l;

		return vec;
	}

	inline Vector3d normalize(const Vector3d& v)
	{
		Vector3d vec = v;
		const double l = length(v);

		vec.x /= l;
		vec.y /= l;
		vec.z /= l;

		return vec;
	}
}

namespace std
{
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
