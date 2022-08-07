#pragma once

#include "Collider2D.h"

#include "Physics/Shapes/BoxShape2D.h"

#include "PolygonCollider2D.h"

#include <memory>

namespace Puffin::Physics::Collision2D
{
	struct BoxCollider2D : public PolygonCollider2D
	{
		BoxCollider2D(ECS::EntityID entity, std::shared_ptr<BoxShape2D> shape) : PolygonCollider2D(entity, shape) {}

		~BoxCollider2D()
		{
			entity = ECS::INVALID_ENTITY;
			shape = nullptr;
		}

		bool TestCollision(const Collision2D::Collider2D* collider, Collision2D::Contact& outContact) const override;
		bool TestCollision(const Collision2D::BoxCollider2D*, Collision2D::Contact& outContact) const override;
		bool TestCollision(const Collision2D::CircleCollider2D*, Collision2D::Contact& outContact) const override;
	};
}