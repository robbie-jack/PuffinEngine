
#include "puffin/physics/onager2d/colliders/circle_collider_2d.h"

#include "puffin/physics/onager2d/physics_helpers_2d.h"
#include "puffin/physics/onager2d/physics_types_2d.h"
#include "puffin/physics/onager2d/colliders/collider_2d.h"

namespace puffin::physics::collision2D
{
	AABB_2D CircleCollider2D::getAABB() const
	{
		return shape->getAABB(position, rotation);
	}

	Vector2f CircleCollider2D::findFurthestPoint(Vector2f direction) const
	{
		return position + direction.normalized() * shape->radius;
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
