#pragma once

#include "polygon_shape_2d.h"
#include "shape_2d.h"
#include "puffin/types/vector.h"

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
            centre_of_mass.zero();
            half_extent.zero();
			points.clear();
		}

		ShapeType2D getType() const override;

		AABB_2D getAABB(const Vector2f& position, const float& rotation) const override;

		// Regenerate points based on half bound
		void updatePoints() override;

        Vector2f half_extent;
	};
}

