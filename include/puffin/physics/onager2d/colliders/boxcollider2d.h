#pragma once

#include "puffin/physics/onager2d/colliders/polygoncollider2d.h"
#include "puffin/physics/onager2d/shapes/boxshape2d.h"
#include "puffin/types/uuid.h"

namespace puffin
{
	namespace physics
	{
		struct BoxShape2D;
	}
}

namespace puffin::physics::collision2D
{
	struct CircleCollider2D;
	struct BoxCollider2D;
	struct Contact;

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
