#pragma once

#include "PolygonShape2D.h"

#include <vector>

namespace puffin::physics
{
	struct BoxShape2D : public PolygonShape2D
	{
	public:

		BoxShape2D()
		{
			halfExtent = Vector2f(1.0f, 1.0f);
			points.reserve(4);
		}

		~BoxShape2D() override
		{
			centreOfMass.Zero();
			halfExtent.Zero();
			points.clear();
		}

		ShapeType2D getType() const override;

		AABB getAABB(const Vector2f& position, const float& rotation) const override;

		// Regenerate points based on half bound
		void updatePoints() override;

		Vector2f halfExtent;
	};
}

