#include "Physics/Colliders/CircleCollider2D.h"

#include "Physics/PhysicsHelpers2D.h"

namespace Puffin::Physics::Collision2D
{
	AABB CircleCollider2D::GetAABB() const
	{
		return shape->GetAABB(transform_);
	}

	bool CircleCollider2D::TestCollision(const Collider2D* collider, Collision2D::Contact& outContact) const
	{
		return collider->TestCollision(this, outContact);
	}

	bool CircleCollider2D::TestCollision(const BoxCollider2D* collider, Collision2D::Contact& outContact) const
	{
		return TestCircleVsBox(this, collider, outContact);
	}

	bool CircleCollider2D::TestCollision(const CircleCollider2D* collider, Collision2D::Contact& outContact) const
	{
		return Collision2D::TestCircleVsCircle(collider, this, outContact);
	}
}
