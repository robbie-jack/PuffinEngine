#pragma once

#include "PolygonCollider2D.h"
#include "Physics/Onager2D/Shapes/BoxShape2D.h"

namespace puffin::physics::collision2D
{
	struct BoxCollider2D : public PolygonCollider2D
	{
		BoxCollider2D(PuffinID uuid, BoxShape2D* shape) : PolygonCollider2D(uuid, shape) {}

		~BoxCollider2D()
		{
			shape = nullptr;
		}

		bool testCollision(const collision2D::Collider2D* collider, collision2D::Contact& contact) const override;
		bool testCollision(const collision2D::BoxCollider2D* collider, collision2D::Contact& contact) const override;
		bool testCollision(const collision2D::CircleCollider2D* collider, collision2D::Contact& contact) const override;
	};
}