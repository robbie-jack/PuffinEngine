#include "physics/onager2d/colliders/box_collider_2d.h"

#include "physics/onager2d/physics_helpers_2d.h"

namespace puffin::physics::collision2D
{
	bool BoxCollider2D::testCollision(const collision2D::Collider2D* collider, collision2D::Contact& contact) const
	{
		return collider->testCollision(this, contact);
	}

	bool BoxCollider2D::testCollision(const collision2D::BoxCollider2D* collider, collision2D::Contact& contact) const
	{
		return collision2D::testBoxVsBox(collider, this, contact);
	}

	bool BoxCollider2D::testCollision(const collision2D::CircleCollider2D* collider, collision2D::Contact& contact) const
	{
		return collision2D::testCircleVsBox(collider, this, contact);
	}
}
