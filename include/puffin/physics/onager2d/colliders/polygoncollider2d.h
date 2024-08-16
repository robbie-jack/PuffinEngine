#pragma once

#include "puffin/physics/onager2d/colliders/collider2d.h"
#include "puffin/physics/onager2d/physicstypes2d.h"

#include "puffin/physics/onager2d/shapes/polygonshape2d.h"

namespace puffin::physics::collision2D
{
	struct PolygonCollider2D : public Collider2D
	{
		PolygonCollider2D(UUID uuid, PolygonShape2D* inShape) : Collider2D(uuid), shape(inShape) {}

		~PolygonCollider2D()
		{
			shape = nullptr;
		}

		AABB2D getAABB() const override;

		Vector2f findFurthestPoint(Vector2f direction) const override;

		PolygonShape2D* shape = nullptr;
	};
}
