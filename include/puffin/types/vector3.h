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

		void Zero()
		{
			x = 0.0f;
			y = 0.0f;
			z = 0.0f;
		}

		T Dot(const Vector3& vec) const
		{
			return x * vec.x + y * vec.y + z * vec.z;
		}

		Vector3 Cross(const Vector3& vec) const
		{
			Vector3 cross;
			cross.x = y * vec.z - z * vec.y;
			cross.y = x * vec.z - z * vec.x;
			cross.z = x * vec.y - y * vec.x;
			return cross;
		}

		T LengthSq() const
		{
			return x * x + y * y + z * z;
		}

		T Length() const
		{
			return sqrt(LengthSq());
		}

		void Normalize()
		{
			T length = Length();

			x /= length;
			y /= length;
			z /= length;
		}

		Vector3 Normalized() const
		{
			T length = Length();

			return Vector3(x / length, y / length, z / length);
		}

		// Json
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Vector3, x, y, z)
	};

	using Vector3f = Vector3<float>;

	template<>
	inline void reflection::RegisterType<Vector3f>()
	{
		entt::meta<Vector3f>()
			.type(entt::hs("Vector3f"))
			.data<&Vector3f::x>(entt::hs("x"))
			.data<&Vector3f::y>(entt::hs("y"))
			.data<&Vector3f::z>(entt::hs("z"));
	}

	namespace serialization
	{
		template<>
		inline void Serialize<Vector3f>(const Vector3f& type, Archive& archive)
		{
			archive.Set("x", type.x);
			archive.Set("y", type.y);
			archive.Set("z", type.z);
		}

		template<>
		inline void Deserialize<Vector3f>(const Archive& archive, Vector3f& type)
		{
			archive.Get("x", type.x);
			archive.Get("y", type.y);
			archive.Get("z", type.z);
		}
	}

	using Vector3d = Vector3<double>;

	template<>
	inline void reflection::RegisterType<Vector3d>()
	{
		entt::meta<Vector3d>()
			.type(entt::hs("Vector3d"))
			.data<&Vector3d::x>(entt::hs("x"))
			.data<&Vector3d::y>(entt::hs("y"))
			.data<&Vector3d::z>(entt::hs("z"));
	}

	namespace serialization
	{
		template<>
		inline void Serialize<Vector3d>(const Vector3d& type, Archive& archive)
		{
			archive.Set("x", type.x);
			archive.Set("y", type.y);
			archive.Set("z", type.z);
		}

		template<>
		inline void Deserialize<Vector3d>(const Archive& archive, Vector3d& type)
		{
			archive.Get("x", type.x);
			archive.Get("y", type.y);
			archive.Get("z", type.z);
		}
	}

	using Vector3i = Vector3<int>;

	template<>
	inline void reflection::RegisterType<Vector3i>()
	{
		entt::meta<Vector3i>()
			.type(entt::hs("Vector3i"))
			.data<&Vector3i::x>(entt::hs("x"))
			.data<&Vector3i::y>(entt::hs("y"))
			.data<&Vector3i::z>(entt::hs("z"));
	}

	namespace serialization
	{
		template<>
		inline void Serialize<Vector3i>(const Vector3i& type, Archive& archive)
		{
			archive.Set("x", type.x);
			archive.Set("y", type.y);
			archive.Set("z", type.z);
		}

		template<>
		inline void Deserialize<Vector3i>(const Archive& archive, Vector3i& type)
		{
			archive.Get("x", type.x);
			archive.Get("y", type.y);
			archive.Get("z", type.z);
		}
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
