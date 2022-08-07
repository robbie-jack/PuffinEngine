#pragma once

#include "Collider2D.h"

#include "Physics/Shapes/PolygonShape2D.h"

#include <memory>

namespace Puffin::Physics::Collision2D
{
	struct PolygonCollider2D : public Collider2D
	{
		PolygonCollider2D(ECS::EntityID entity, std::shared_ptr<PolygonShape2D> inShape) : Collider2D(entity), shape(inShape) {}

		~PolygonCollider2D()
		{
			entity = ECS::INVALID_ENTITY;
			shape = nullptr;
		}

		AABB GetAABB() const override;

		Vector2f FindFurthestPoint(Vector2f direction) const override;

		std::shared_ptr<PolygonShape2D> shape = nullptr;
	};
}