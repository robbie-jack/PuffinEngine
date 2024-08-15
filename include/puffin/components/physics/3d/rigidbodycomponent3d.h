#pragma once

#include "puffin/types/vector.h"
#include "puffin/physics/bodytype.h"

namespace puffin::physics
{
	struct RigidbodyComponent3D
	{
		RigidbodyComponent3D() = default;

		RigidbodyComponent3D(const BodyType& bodyType_, const float& mass_) : body_type(bodyType_), mass(mass_) {}

		Vector3f linear_velocity = Vector3f(0.0f);
		//float angularVelocity = 0.0f;

		float mass = 0.0f;
		float elasticity = 1.0f;

		BodyType body_type = BodyType::Static;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(RigidbodyComponent3D, mass, elasticity, body_type)
	};
}
