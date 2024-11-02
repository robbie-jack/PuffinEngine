#pragma once

#include "puffin/physics/onager2d/shapes/shape2d.h"
#include "puffin/physics/onager2d/physicstypes2d.h"

namespace puffin::physics
{
	struct CircleShape2D : public Shape2D
	{
		CircleShape2D()
		{
			radius = 1.0f;
		}

		~CircleShape2D() override 
		{
			radius = 0.0f;
            centreOfMass.Zero();
		}

		ShapeType2D GetType() const override;

		AABB2D GetAABB(const Vector2f& position, const float& rotation) const override;

		float radius;
	};
}
