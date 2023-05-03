#pragma once

#include "Collider2D.h"

#include "Physics/Onager2D/Shapes/PolygonShape2D.h"

namespace Puffin::Physics::Collision2D
{
	struct PolygonCollider2D : public Collider2D
	{
		PolygonCollider2D(UUID uuid, PolygonShape2D* inShape) : Collider2D(uuid), shape(inShape) {}

		~PolygonCollider2D()
		{
			shape = nullptr;
		}

		AABB GetAABB() const override;

		Vector2f FindFurthestPoint(Vector2f direction) const override;

		PolygonShape2D* shape = nullptr;
	};
}