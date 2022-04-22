#pragma once

#include "Physics/Shapes/Shape2D.h"

namespace Puffin::Physics
{
	struct CircleShape2D : public Shape2D
	{
		CircleShape2D() : Shape2D()
		{
			radius_ = 2.f;
		}

		ShapeType2D GetType() const override;

		AABB GetAABB(const TransformComponent& transform) const;

		float radius_;

		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(centreOfMass_);
			archive(radius_);
		}
	};
}