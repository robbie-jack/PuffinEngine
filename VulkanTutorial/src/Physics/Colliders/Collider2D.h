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
		Collider2D(ECS::Entity inEntity) : entity(inEntity) {}

		virtual ~Collider2D() = default;

		// Get AABB representing the maximum bounds of this shape
		virtual AABB GetAABB() const = 0;

		virtual Vector2f FindFurthestPoint(Vector2f direction) const = 0;

		virtual bool TestCollision(const Collision2D::Collider2D* collider, Collision2D::Contact& outContact) const = 0;
		virtual bool TestCollision(const Collision2D::BoxCollider2D* collider, Collision2D::Contact& outContact) const = 0;
		virtual bool TestCollision(const Collision2D::CircleCollider2D* collider, Collision2D::Contact& outContact) const = 0;

		ECS::Entity entity;
		TransformComponent transform;
	};
}