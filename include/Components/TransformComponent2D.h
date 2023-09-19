#pragma once

#include "Types/Vector.h"

#include "nlohmann/json.hpp"

namespace puffin
{
	struct TransformComponent2D
	{
		TransformComponent2D() = default;

#ifdef PFN_DOUBLE_PRECISION
		TransformComponent2D(const Vector2d& position_) : position(position_) {}

		TransformComponent2D(const Vector2d& position_, const float& rotation_, const Vector2f& scale_) :
			position(position_), rotation(rotation_), scale(scale_) {}
#else
		TransformComponent2D(const Vector2f& position_) : position(position_) {}

		TransformComponent2D(const Vector2f& position_, const float& rotation_, const Vector2f& scale_) :
			position(position_), rotation(rotation_), scale(scale_) {}
#endif

		TransformComponent2D(const TransformComponent2D& t) : position(t.position), rotation(t.rotation), scale(t.scale) {}

		~TransformComponent2D() = default;

		TransformComponent2D& operator=(const TransformComponent2D& rhs) = default;

#ifdef PFN_DOUBLE_PRECISION
		Vector2d position = Vector2d(0.0);
#else
		Vector2f position = Vector2f(0.0f);
#endif

		float rotation = 0.0;

		Vector2f scale = Vector2f(1.0f);

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(TransformComponent2D, position, rotation, scale)
	};
}