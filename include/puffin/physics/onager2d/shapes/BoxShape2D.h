#pragma once

#include "puffin/physics/onager2d/shapes/polygonshape2d.h"
#include "puffin/physics/onager2d/shapes/shape2d.h"
#include "puffin/types/vector2.h"

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

		ShapeType2D getType() const override;

		AABB2D getAABB(const Vector2f& position, const float& rotation) const override;

		// Regenerate points based on half bound
		void updatePoints() override;

        Vector2f half_extent;
	};
}

