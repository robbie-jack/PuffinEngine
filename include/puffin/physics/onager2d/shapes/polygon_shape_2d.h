#pragma once

#include <vector>

#include "puffin/physics/onager2d/shapes/shape_2d.h"

namespace puffin::physics
{
	struct PolygonShape2D : public Shape2D
	{
		PolygonShape2D() : Shape2D() {}

		~PolygonShape2D()
		{
            centre_of_mass.zero();
			points.clear();
		}

		AABB_2D getAABB(const Vector2f& position, const float& rotation) const = 0;

		virtual void updatePoints() = 0;

		std::vector<Vector2f> points;
	};
}
