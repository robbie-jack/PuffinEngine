#pragma once

#include <cassert>
#include <corecrt_math.h>
#include <valarray>

#include "glm/vec2.hpp"

#ifdef PFN_BOX2D_PHYSICS
#include "box2d/math_functions.h"
#endif

#include "nlohmann/json.hpp"

#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"

namespace puffin
{
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
		bool operator== (const Vector2<float>& vec) const
		{
			return x == vec.x && y == vec.y;
		}

		bool operator== (const Vector2<double>& vec) const
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

#ifdef PFN_BOX2D_PHYSICS
		explicit operator b2Vec2() const
		{
			b2Vec2 vec;
			vec.x = x;
			vec.y = y;
			return vec;
		}
#endif

		explicit operator Vector2<float>() const
		{
			return Vector2<float>(x, y);
		}

		explicit operator Vector2<double>() const
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

		void operator*= (const Vector2& vec)
		{
			x *= vec.x;
			y *= vec.y;
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
			assert(idx >= 0 && idx < 2 && "Vector2::operator[] - Idx not between 0 and 2");

			return (&x)[idx];
		}

		T& operator[] (const int idx)
		{
			assert(idx >= 0 && idx < 2 && "Vector2::operator[] - Idx not between 0 and 2");

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

		[[nodiscard]] Vector2<T> PerpendicularClockwise() const
		{
			return Vector2<T>(y, -x);
		}

		[[nodiscard]] Vector2<T> PerpendicularCounterClockwise() const
		{
			return Vector2<T>(-y, x);
		}

		T LengthSq() const
		{
			return x * x + y * y;
		}

		T Length() const
		{
			return sqrtf(LengthSq());
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

		void Normalize()
		{
			T length = Length();

			x /= length;
			y /= length;
		}

		[[nodiscard]] Vector2 Normalized() const
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

		[[nodiscard]] Vector2 Abs() const
		{
			return Vector2(std::abs(x), std::abs(y));
		}

		// Json
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Vector2, x, y)
	};

	using Vector2f = Vector2<float>;

	template<>
	inline void reflection::RegisterType<Vector2f>()
	{
		entt::meta<Vector2f>()
			.type(entt::hs("Vector2f"))
			.data<&Vector2f::x>(entt::hs("x"))
			.data<&Vector2f::y>(entt::hs("y"));
	}

	namespace serialization
	{
		template<>
		inline nlohmann::json Serialize<Vector2f>(const Vector2f& data)
		{
			nlohmann::json json;
			json["x"] = data.x;
			json["y"] = data.y;
			return json;
		}

		template<>
		inline Vector2f Deserialize<Vector2f>(const nlohmann::json& json)
		{
			Vector2f data;
			data.x = json["x"];
			data.y = json["y"];
			return data;
		}
	}

	using Vector2d = Vector2<double>;

	template<>
	inline void reflection::RegisterType<Vector2d>()
	{
		entt::meta<Vector2d>()
			.type(entt::hs("Vector2d"))
			.data<&Vector2d::x>(entt::hs("x"))
			.data<&Vector2d::y>(entt::hs("y"));
	}

	namespace serialization
	{
		template<>
		inline nlohmann::json Serialize<Vector2d>(const Vector2d& data)
		{
			nlohmann::json json;
			json["x"] = data.x;
			json["y"] = data.y;
			return json;
		}

		template<>
		inline Vector2d Deserialize<Vector2d>(const nlohmann::json& json)
		{
			Vector2d data;
			data.x = json["x"];
			data.y = json["y"];
			return data;
		}
	}

	using Vector2i = Vector2<int>;

	template<>
	inline void reflection::RegisterType<Vector2i>()
	{
		entt::meta<Vector2i>()
			.type(entt::hs("Vector2i"))
			.data<&Vector2i::x>(entt::hs("x"))
			.data<&Vector2i::y>(entt::hs("y"));
	}

	namespace serialization
	{
		template<>
		inline nlohmann::json Serialize<Vector2i>(const Vector2i& data)
		{
			nlohmann::json json;
			json["x"] = data.x;
			json["y"] = data.y;
			return json;
		}

		template<>
		inline Vector2i Deserialize<Vector2i>(const nlohmann::json& json)
		{
			Vector2i data;
			data.x = json["x"];
			data.y = json["y"];
			return data;
		}
	}
}

template <>
struct std::hash<puffin::Vector2f>
{
	size_t operator()(const puffin::Vector2f& vec) const noexcept
	{
		return (hash<float>()(vec.x) ^
			(hash<float>()(vec.y) << 1) >> 1);
	}
};

template <>
struct std::hash<puffin::Vector2d>
{
	size_t operator()(const puffin::Vector2d& vec) const noexcept
	{
		return (hash<double>()(vec.x) ^
			(hash<double>()(vec.y) << 1) >> 1);
	}
};

template <>
struct std::hash<puffin::Vector2i>
{
	size_t operator()(const puffin::Vector2i& vec) const noexcept
	{
		return (hash<int>()(vec.x) ^
			(hash<int>()(vec.y) << 1) >> 1);
	}
};
