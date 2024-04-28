#pragma once

#include "puffin/types/vector.h"
#include "puffin/physics/body_type.h"

namespace puffin::physics
{
	struct RigidbodyComponent2D
	{
		RigidbodyComponent2D() = default;

		RigidbodyComponent2D(const BodyType& bodyType_, const float& mass_) : bodyType(bodyType_), mass(mass_) {}

		Vector2f linearVelocity = Vector2f(0.0f);
		float angularVelocity = 0.0f;

		float mass = 0.0f;
		float elasticity = 1.0f;

		BodyType bodyType = BodyType::Static;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(RigidbodyComponent2D, mass, elasticity, bodyType)
	};
}
