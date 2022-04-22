#pragma once

#include "Shape2D.h"

namespace Puffin::Physics
{
	struct BoxShape2D : public Shape2D
	{
	public:

		BoxShape2D() : Shape2D()
		{
			halfExtent_ = Vector2f(.5f, .5f);
		}

		ShapeType2D GetType() const override;

		AABB GetAABB(const TransformComponent& transform) const;

		Vector2f halfExtent_;

		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(centreOfMass_);
			archive(halfExtent_);
		}
	};
}

