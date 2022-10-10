#pragma once

#include "PolygonShape2D.h"

#include <vector>

namespace Puffin::Physics
{
	struct BoxShape2D : public PolygonShape2D
	{
	public:

		BoxShape2D() : PolygonShape2D()
		{
			halfExtent = Vector2f(1.0f, 1.0f);
			points.reserve(4);
		}

		~BoxShape2D()
		{
			centreOfMass.Zero();
			halfExtent.Zero();
			points.clear();
		}

		ShapeType2D GetType() const override;

		AABB GetAABB(const Vector2f& position, const float& rotation) const;

		// Regenerate points based on half bound
		void UpdatePoints() override;

		Vector2f halfExtent;
	};
}

