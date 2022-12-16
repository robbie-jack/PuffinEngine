#pragma once

#include "Physics/Onager2D/Shapes/Shape2D.h"

namespace Puffin::Physics
{
	struct CircleShape2D : public Shape2D
	{
		CircleShape2D() : Shape2D()
		{
			radius = 1.0f;
		}

		~CircleShape2D() 
		{
			radius = 0.0f;
			centreOfMass.Zero();
		}

		ShapeType2D GetType() const override;

		AABB GetAABB(const Vector2f& position, const float& rotation) const;

		float radius;
	};
}