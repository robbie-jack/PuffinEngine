#include "Physics/Colliders/BoxCollider2D.h"

#include "Physics/PhysicsHelpers2D.h"

namespace Puffin::Physics::Collision2D
{
	AABB BoxCollider2D::GetAABB() const
	{
		return shape_->GetAABB(transform_);
	}

	bool BoxCollider2D::TestCollision(const Collider2D* collider, Collision2D::Contact& outContact) const
	{
		return collider->TestCollision(this, outContact);
	}

	bool BoxCollider2D::TestCollision(const BoxCollider2D* collider, Collision2D::Contact& outContact) const
	{
		return Collision2D::TestBoxVsBox(collider, this, outContact);
	}

	bool BoxCollider2D::TestCollision(const CircleCollider2D* collider, Collision2D::Contact& outContact) const
	{
		return Collision2D::TestCircleVsBox(collider, this, outContact);
	}
}
