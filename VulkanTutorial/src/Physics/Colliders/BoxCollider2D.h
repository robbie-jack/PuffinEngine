#pragma once

#include "Physics/Colliders/Collider2D.h"
#include "Physics/Shapes/BoxShape2D.h"

#include <memory>

namespace Puffin::Physics::Collision2D
{
	struct BoxCollider2D : public Collider2D
	{
		BoxCollider2D(ECS::Entity entity, BoxShape2D* shape) : Collider2D(entity), shape_(shape) {}

		AABB GetAABB() const override;

		bool TestCollision(const Collider2D* collider, Collision2D::Contact& outContact) const override;
		bool TestCollision(const BoxCollider2D* collider, Collision2D::Contact& outContact) const override;
		bool TestCollision(const CircleCollider2D* collider, Collision2D::Contact& outContact) const override;

		BoxShape2D* shape_;
	};
}