#pragma once

#include "puffin/types/vector.h"
#include "puffin/physics/body_type.h"

namespace puffin::physics
{
	struct RigidbodyComponent2D
	{
		RigidbodyComponent2D() = default;

		RigidbodyComponent2D(const BodyType& bodyType_, const float& mass_) : body_type(bodyType_), mass(mass_) {}

		Vector2f linear_velocity = Vector2f(0.0f);
		float angular_velocity = 0.0f;

		float mass = 0.0f;
		float elasticity = 1.0f;

		BodyType body_type = BodyType::Static;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(RigidbodyComponent2D, mass, elasticity, body_type)
	};
}
