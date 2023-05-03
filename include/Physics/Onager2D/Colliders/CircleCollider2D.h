#pragma once

#include "Physics/Onager2D/Colliders/Collider2D.h"
#include "Physics/Onager2D/Shapes/CircleShape2D.h"

namespace Puffin::Physics::Collision2D
{
	struct CircleCollider2D : public Collider2D
	{
		CircleCollider2D(UUID uuid, CircleShape2D* shape) : Collider2D(uuid), shape(shape) {}

		~CircleCollider2D()
		{
			shape = nullptr;
		}

		AABB GetAABB() const override;

		Vector2f FindFurthestPoint(Vector2f direction) const override;

		bool TestCollision(const Collision2D::Collider2D* collider, Collision2D::Contact& outContact) const override;
		bool TestCollision(const Collision2D::BoxCollider2D*, Collision2D::Contact& outContact) const override;
		bool TestCollision(const Collision2D::CircleCollider2D*, Collision2D::Contact& outContact) const override;

		CircleShape2D* shape = nullptr;
	};
}