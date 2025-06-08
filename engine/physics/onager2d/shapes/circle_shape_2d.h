#pragma once

#include "physics/onager2d/shapes/shape_2d.h"
#include "physics/onager2d/physics_types_2d.h"

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
