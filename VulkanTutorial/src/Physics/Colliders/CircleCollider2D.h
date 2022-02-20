#pragma once

#include "Physics/Colliders/Collider2D.h"
#include "Physics/Shapes/CircleShape2D.h"

namespace Puffin::Physics::Collision2D
{
	struct CircleCollider2D : public Collider2D
	{
		CircleCollider2D(ECS::Entity entity, CircleShape2D* shape) : Collider2D(entity), shape_(shape) {}

		AABB GetAABB() const override;

		bool TestCollision(const Collider2D* collider, Collision2D::Contact& outContact) const override;
		bool TestCollision(const BoxCollider2D* collider, Collision2D::Contact& outContact) const override;
		bool TestCollision(const CircleCollider2D* collider, Collision2D::Contact& outContact) const override;

		CircleShape2D* shape_;
	};
}