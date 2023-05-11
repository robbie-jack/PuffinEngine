#pragma once

#include "Types/Vector.h"

#include "nlohmann/json.hpp"

namespace puffin
{
	namespace physics
	{
		enum class BodyType
		{
			Static = 0,
			Kinematic = 1,
			Dynamic = 2
		};

		NLOHMANN_JSON_SERIALIZE_ENUM(BodyType,
		{
			{BodyType::Static, "Static"},
			{BodyType::Kinematic, "Kinematic"},
			{BodyType::Dynamic, "Dynamic"}
		})

		struct RigidbodyComponent2D
		{
			RigidbodyComponent2D() = default;

			RigidbodyComponent2D(const BodyType bodyType_, const float mass_) : bodyType(bodyType_), mass(mass_) {}

			Vector2f linearVelocity = Vector2f(0.0f);
			float angularVelocity = 0.0f;

			float mass = 0.0f;
			float elasticity = 1.0f;

			BodyType bodyType = BodyType::Static;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(RigidbodyComponent2D, mass, elasticity, bodyType)
		};
	}
}