#include "Physics/Colliders/CircleCollider2D.h"

#include "Physics/PhysicsHelpers2D.h"

namespace Puffin::Physics::Collision2D
{
	AABB CircleCollider2D::GetAABB() const
	{
		return shape->GetAABB(transform);
	}

	Vector2f CircleCollider2D::FindFurthestPoint(Vector2f direction) const
	{
		return transform.position.GetXY() + direction.Normalised() * shape->radius;
	}

	bool CircleCollider2D::TestCollision(const Collision2D::Collider2D* collider, Collision2D::Contact& outContact) const
	{
		return collider->TestCollision(this, outContact);
	}

	bool CircleCollider2D::TestCollision(const Collision2D::BoxCollider2D* collider, Collision2D::Contact& outContact) const
	{
		return Collision2D::TestCircleVsBox(this, collider, outContact);
	}

	bool CircleCollider2D::TestCollision(const Collision2D::CircleCollider2D* collider, Collision2D::Contact& outContact) const
	{
		return Collision2D::TestCircleVsCircle(collider, this, outContact);
	}
}
