#pragma once

#include "Physics/Colliders/Collider2D.h"
#include "Physics/Shapes/CircleShape2D.h"

#include <memory>

namespace Puffin::Physics::Collision2D
{
	struct CircleCollider2D : public Collider2D
	{
		CircleCollider2D(ECS::Entity entity, std::shared_ptr<CircleShape2D> shape) : Collider2D(entity), shape(shape) {}

		~CircleCollider2D()
		{
			entity = ECS::INVALID_ENTITY;
			shape = nullptr;
		}

		AABB GetAABB() const override;

		Vector2f FindFurthestPoint(Vector2f direction) const override;

		bool TestCollision(const Collision2D::Collider2D* collider, Collision2D::Contact& outContact) const override;
		bool TestCollision(const Collision2D::BoxCollider2D*, Collision2D::Contact& outContact) const override;
		bool TestCollision(const Collision2D::CircleCollider2D*, Collision2D::Contact& outContact) const override;

		std::shared_ptr<CircleShape2D> shape = nullptr;
	};
}