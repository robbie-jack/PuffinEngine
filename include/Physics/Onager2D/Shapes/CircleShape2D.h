#pragma once

#include "Physics/Onager2D/Shapes/Shape2D.h"

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

		ShapeType2D getType() const override;

		AABB getAABB(const Vector2f& position, const float& rotation) const override;

		float radius;
	};
}