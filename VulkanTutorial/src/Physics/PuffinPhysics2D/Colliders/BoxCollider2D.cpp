#include "Physics/Colliders/BoxCollider2D.h"

#include "Physics/PhysicsHelpers2D.h"

namespace Puffin::Physics::Collision2D
{
	bool BoxCollider2D::TestCollision(const Collision2D::Collider2D* collider, Collision2D::Contact& outContact) const
	{
		return collider->TestCollision(this, outContact);
	}

	bool BoxCollider2D::TestCollision(const Collision2D::BoxCollider2D* collider, Collision2D::Contact& outContact) const
	{
		return Collision2D::TestBoxVsBox(collider, this, outContact);
	}

	bool BoxCollider2D::TestCollision(const Collision2D::CircleCollider2D* collider, Collision2D::Contact& outContact) const
	{
		return Collision2D::TestCircleVsBox(collider, this, outContact);
	}
}
