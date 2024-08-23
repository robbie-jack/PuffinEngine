#pragma once

#include "puffin/types/vector2.h"
#include "puffin/physics/bodytype.h"
#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"

namespace puffin
{
	namespace physics
	{
		struct RigidbodyComponent2D
		{
			RigidbodyComponent2D() = default;

			RigidbodyComponent2D(const BodyType& bodyType, const float& mass) : bodyType(bodyType), mass(mass) {}

			BodyType bodyType = BodyType::Static;

			float mass = 0.0f;
			float elasticity = 1.0f;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(RigidbodyComponent2D, mass, elasticity, bodyType)
		};
	}

	template<>
	inline void reflection::RegisterType<physics::RigidbodyComponent2D>()
	{
		using namespace physics;

		entt::meta<RigidbodyComponent2D>()
			.type(entt::hs("RigidbodyComponent2D"))
			.data<&RigidbodyComponent2D::bodyType>(entt::hs("bodyType"))
			.data<&RigidbodyComponent2D::mass>(entt::hs("mass"))
			.data<&RigidbodyComponent2D::elasticity>(entt::hs("elasticity"));
	}

	namespace serialization
	{
		template<>
		inline void Serialize<physics::RigidbodyComponent2D>(const physics::RigidbodyComponent2D& data, Archive& archive)
		{
			archive.Set("bodyType", static_cast<uint8_t>(data.bodyType));
			archive.Set("mass", data.mass);
			archive.Set("elasticity", data.elasticity);
		}

		template<>
		inline void Deserialize<physics::RigidbodyComponent2D>(const Archive& archive, physics::RigidbodyComponent2D& data)
		{
			uint8_t bodyType;
			archive.Get("bodyType", bodyType);
			data.bodyType = static_cast<physics::BodyType>(bodyType);

			archive.Get("mass", data.mass);
			archive.Get("elasticity", data.elasticity);
		}
	}
}
