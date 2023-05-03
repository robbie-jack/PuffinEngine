#pragma once

#include "PolygonCollider2D.h"
#include "Physics/Onager2D/Shapes/BoxShape2D.h"

namespace Puffin::Physics::Collision2D
{
	struct BoxCollider2D : public PolygonCollider2D
	{
		BoxCollider2D(UUID uuid, BoxShape2D* shape) : PolygonCollider2D(uuid, shape) {}

		~BoxCollider2D()
		{
			shape = nullptr;
		}

		bool TestCollision(const Collision2D::Collider2D* collider, Collision2D::Contact& outContact) const override;
		bool TestCollision(const Collision2D::BoxCollider2D*, Collision2D::Contact& outContact) const override;
		bool TestCollision(const Collision2D::CircleCollider2D*, Collision2D::Contact& outContact) const override;
	};
}