#pragma once

#include "puffin/physics/onager2d/colliders/collider_2d.h"
#include "puffin/physics/onager2d/physics_types_2d.h"

#include "puffin/physics/onager2d/shapes/polygon_shape_2d.h"

namespace puffin::physics::collision2D
{
	struct PolygonCollider2D : public Collider2D
	{
		PolygonCollider2D(PuffinID uuid, PolygonShape2D* inShape) : Collider2D(uuid), shape(inShape) {}

		~PolygonCollider2D()
		{
			shape = nullptr;
		}

		AABB getAABB() const override;

		Vector2f findFurthestPoint(Vector2f direction) const override;

		PolygonShape2D* shape = nullptr;
	};
}
