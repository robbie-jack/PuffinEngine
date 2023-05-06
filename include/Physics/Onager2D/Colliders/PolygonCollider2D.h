#pragma once

#include "Collider2D.h"

#include "Physics/Onager2D/Shapes/PolygonShape2D.h"

namespace puffin::physics::collision2D
{
	struct PolygonCollider2D : public Collider2D
	{
		PolygonCollider2D(PuffinId uuid, PolygonShape2D* inShape) : Collider2D(uuid), shape(inShape) {}

		~PolygonCollider2D()
		{
			shape = nullptr;
		}

		AABB getAABB() const override;

		Vector2f findFurthestPoint(Vector2f direction) const override;

		PolygonShape2D* shape = nullptr;
	};
}