#pragma once

#include "puffin/types/vector.h"
#include "puffin/physics/bodytype.h"

namespace puffin::physics
{
	struct RigidbodyComponent2D
	{
		RigidbodyComponent2D() = default;

		RigidbodyComponent2D(const BodyType& bodyType, const float& mass) : mass(mass), bodyType(bodyType) {}

		Vector2f linearVelocity = Vector2f(0.0f);
		float angularVelocity = 0.0f;

		float mass = 0.0f;
		float elasticity = 1.0f;

		BodyType bodyType = BodyType::Static;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(RigidbodyComponent2D, mass, elasticity, bodyType)
	};
}
