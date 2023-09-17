#pragma once

#include "Types/Vector.h"
#include "Physics/BodyType.h"

namespace puffin::physics
{
	struct RigidbodyComponent3D
	{
		RigidbodyComponent3D() = default;

		RigidbodyComponent3D(const BodyType& bodyType_, const float& mass_) : bodyType(bodyType_), mass(mass_) {}

		Vector3f linearVelocity = Vector3f(0.0f);
		//float angularVelocity = 0.0f;

		float mass = 0.0f;
		float elasticity = 1.0f;

		BodyType bodyType = BodyType::Static;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(RigidbodyComponent3D, mass, elasticity, bodyType)
	};
}