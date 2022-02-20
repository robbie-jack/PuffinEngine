#pragma once

#include "Components/TransformComponent.h"
#include "Physics/PhysicsTypes2D.h"
#include "ECS/ECS.h"

namespace Puffin::Physics::Collision2D
{
	struct BoxCollider2D;
	struct CircleCollider2D;

	struct Collider2D
	{
		Collider2D(ECS::Entity entity) : entity_(entity) {}

		virtual ~Collider2D() = default;

		virtual AABB GetAABB() const = 0;

		virtual bool TestCollision(const Collider2D* collider, Collision2D::Contact& outContact) const = 0;
		virtual bool TestCollision(const BoxCollider2D* collider, Collision2D::Contact& outContact) const = 0;
		virtual bool TestCollision(const CircleCollider2D* collider, Collision2D::Contact& outContact) const = 0;

		ECS::Entity entity_;
		TransformComponent transform_;
	};
}