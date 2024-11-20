#pragma once

#include "puffin/types/vector3.h"
#include "puffin/physics/bodytype.h"
#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"

namespace puffin
{
	namespace physics
	{
		struct RigidbodyComponent3D
		{
			RigidbodyComponent3D() = default;

			RigidbodyComponent3D(const BodyType& bodyType, const float& mass) : bodyType(bodyType), mass(mass) {}

			BodyType bodyType = BodyType::Static;

			float mass = 0.0f;
			float density = 1.0f;
			float elasticity = 1.0f;
			float friction = 0.5f;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(RigidbodyComponent3D, bodyType, mass, density, elasticity, friction)
		};
	}

	template<>
	inline void reflection::RegisterType<physics::RigidbodyComponent3D>()
	{
		using namespace physics;

		entt::meta<RigidbodyComponent3D>()
			.type(entt::hs("RigidbodyComponent3D"))
			.data<&RigidbodyComponent3D::bodyType>(entt::hs("bodyType"))
			.data<&RigidbodyComponent3D::mass>(entt::hs("mass"))
			.data<&RigidbodyComponent3D::elasticity>(entt::hs("elasticity"));
	}

	namespace serialization
	{
		template<>
		inline void Serialize<physics::RigidbodyComponent3D>(const physics::RigidbodyComponent3D& data, Archive& archive)
		{
			archive.Set("bodyType", static_cast<uint8_t>(data.bodyType));
			archive.Set("mass", data.mass);
			archive.Set("elasticity", data.elasticity);
		}

		template<>
		inline void Deserialize<physics::RigidbodyComponent3D>(const Archive& archive, physics::RigidbodyComponent3D& data)
		{
			uint8_t bodyType;
			archive.Get("bodyType", bodyType);
			data.bodyType = static_cast<physics::BodyType>(bodyType);

			archive.Get("mass", data.mass);
			archive.Get("elasticity", data.elasticity);
		}

		template<>
		inline nlohmann::json Serialize<physics::RigidbodyComponent3D>(const physics::RigidbodyComponent3D& data)
		{
			nlohmann::json json;
			json["bodyType"] = physics::gBodyTypeToString.at(data.bodyType);
			json["mass"] = data.mass;
			json["density"] = data.density;
			json["elasticity"] = data.density;
			json["friction"] = data.density;
			return json;
		}

		template<>
		inline physics::RigidbodyComponent3D Deserialize<physics::RigidbodyComponent3D>(const nlohmann::json& json)
		{
			physics::RigidbodyComponent3D data;
			data.bodyType = physics::gStringToBodyType.at(json["bodyType"]);
			data.mass = json["centreOfMass"];
			data.density = json["density"];
			data.elasticity = json["elasticity"];
			data.friction = json["friction"];
			return data;
		}
	}
}
