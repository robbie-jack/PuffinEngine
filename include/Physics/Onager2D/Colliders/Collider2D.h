#pragma once

#include "Physics/Onager2D/PhysicsTypes2D.h"
#include "Types/UUID.h"

namespace puffin::physics::collision2D
{
	struct BoxCollider2D;
	struct CircleCollider2D;

	struct Collider2D
	{
		Collider2D(PuffinID inUUID) : uuid(inUUID), rotation(0.0f) {}

		virtual ~Collider2D() = default;

		// Get AABB representing the maximum bounds of this shape
		virtual AABB getAABB() const = 0;

		virtual Vector2f findFurthestPoint(Vector2f direction) const = 0;

		virtual bool testCollision(const collision2D::Collider2D* collider, collision2D::Contact& contact) const = 0;
		virtual bool testCollision(const collision2D::BoxCollider2D* collider, collision2D::Contact& outContact) const = 0;
		virtual bool testCollision(const collision2D::CircleCollider2D* collider, collision2D::Contact& outContact) const = 0;

		PuffinID uuid;

		Vector2f position;
		float rotation;
	};
}