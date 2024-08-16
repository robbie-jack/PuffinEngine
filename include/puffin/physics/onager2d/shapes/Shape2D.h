#pragma once

#include "puffin/types/vector.h"

namespace puffin
{
	struct AABB2D;
};

namespace puffin::physics
{
	enum class ShapeType2D : uint8_t
	{
		box = 1,
		circle = 2
	};

	struct Shape2D
	{
		Shape2D()
		{
            centre_of_mass.zero();
		}

		virtual ~Shape2D() = default;

		virtual ShapeType2D getType() const = 0;

		virtual AABB2D getAABB(const Vector2f& position, const float& rotation) const = 0;

        Vector2f centre_of_mass; // Centre of mass for this shape
	};
}
