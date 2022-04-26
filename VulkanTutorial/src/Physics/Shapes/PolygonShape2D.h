#pragma once

#include "Shape2D.h"

#include <vector>

namespace Puffin::Physics
{
	struct PolygonShape2D : public Shape2D
	{
		PolygonShape2D() : Shape2D() {}

		~PolygonShape2D()
		{
			points.clear();
		}

		AABB GetAABB(const TransformComponent& transform) const = 0;

		virtual void UpdatePoints() = 0;

		std::vector<Vector2f> points;
	};
}