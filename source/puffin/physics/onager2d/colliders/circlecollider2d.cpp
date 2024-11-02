
#include "puffin/physics/onager2d/colliders/circlecollider2d.h"

#include "puffin/physics/onager2d/physicshelpers2d.h"
#include "puffin/physics/onager2d/physicstypes2d.h"
#include "puffin/physics/onager2d/colliders/collider2d.h"

namespace puffin::physics::collision2D
{
	AABB2D CircleCollider2D::getAABB() const
	{
		return shape->GetAABB(position, rotation);
	}

	Vector2f CircleCollider2D::findFurthestPoint(Vector2f direction) const
	{
		return position + direction.Normalized() * shape->radius;
	}

	bool CircleCollider2D::testCollision(const collision2D::Collider2D* collider, collision2D::Contact& contact) const
	{
		return collider->testCollision(this, contact);
	}

	bool CircleCollider2D::testCollision(const collision2D::BoxCollider2D* collider, collision2D::Contact& contact) const
	{
		return collision2D::testCircleVsBox(this, collider, contact);
	}

	bool CircleCollider2D::testCollision(const collision2D::CircleCollider2D* collider, collision2D::Contact& contact) const
	{
		return collision2D::testCircleVsCircle(collider, this, contact);
	}
}
