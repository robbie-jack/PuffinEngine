#pragma once

#include "physics/onager2d/shapes/polygon_shape_2d.h"
#include "physics/onager2d/shapes/shape_2d.h"
#include "types/vector2.h"

namespace puffin::physics
{
	struct BoxShape2D : public PolygonShape2D
	{
	public:

		BoxShape2D()
		{
            half_extent = Vector2f(1.0f, 1.0f);
			points.reserve(4);
		}

		~BoxShape2D() override
		{
            centreOfMass.Zero();
            half_extent.Zero();
			points.clear();
		}

		ShapeType2D GetType() const override;

		AABB2D GetAABB(const Vector2f& position, const float& rotation) const override;

		// Regenerate points based on half bound
		void UpdatePoints() override;

        Vector2f half_extent;
	};
}

