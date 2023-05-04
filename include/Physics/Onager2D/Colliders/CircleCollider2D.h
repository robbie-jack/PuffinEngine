#pragma once

#include "Physics/Onager2D/Colliders/Collider2D.h"
#include "Physics/Onager2D/Shapes/CircleShape2D.h"

namespace puffin::physics::collision2D
{
	struct CircleCollider2D : public Collider2D
	{
		CircleCollider2D(UUID uuid, CircleShape2D* shape) : Collider2D(uuid), shape(shape) {}

		~CircleCollider2D() override
		{
			shape = nullptr;
		}

		AABB getAABB() const override;

		Vector2f findFurthestPoint(Vector2f direction) const override;

		bool testCollision(const collision2D::Collider2D* collider, collision2D::Contact& contact) const override;
		bool testCollision(const collision2D::BoxCollider2D* collider, collision2D::Contact& contact) const override;
		bool testCollision(const collision2D::CircleCollider2D* collider, collision2D::Contact& contact) const override;

		CircleShape2D* shape = nullptr;
	};
}