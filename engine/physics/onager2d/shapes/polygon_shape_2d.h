#pragma once

#include <vector>

#include "physics/onager2d/shapes/shape_2d.h"

namespace puffin::physics
{
	struct PolygonShape2D : public Shape2D
	{
		PolygonShape2D() : Shape2D() {}

		~PolygonShape2D()
		{
            centreOfMass.Zero();
			points.clear();
		}

		AABB2D GetAABB(const Vector2f& position, const float& rotation) const = 0;

		virtual void UpdatePoints() = 0;

		std::vector<Vector2f> points;
	};
}
