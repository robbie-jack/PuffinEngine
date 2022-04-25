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

		ShapeType2D GetType() const override;

		AABB GetAABB(const TransformComponent& transform) const;

		// Regenerate points based on half bound
		void UpdatePoints() override;

		Vector2f halfExtent;

		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(centreOfMass);
			archive(halfExtent);
		}
	};
}

