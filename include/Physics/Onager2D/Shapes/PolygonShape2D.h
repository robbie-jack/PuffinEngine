#pragma once

#include "Shape2D.h"

#include <vector>

namespace puffin::physics
{
	struct PolygonShape2D : public Shape2D
	{
		PolygonShape2D() : Shape2D() {}

		~PolygonShape2D()
		{
			centreOfMass.zero();
			points.clear();
		}

		AABB getAABB(const Vector2f& position, const float& rotation) const = 0;

		virtual void updatePoints() = 0;

		std::vector<Vector2f> points;
	};
}