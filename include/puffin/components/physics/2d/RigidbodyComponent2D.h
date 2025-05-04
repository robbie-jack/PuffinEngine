#pragma once

#include "puffin/types/vector2.h"
#include "puffin/physics/bodytype.h"
#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"
#include "puffin/serialization/componentserialization.h"

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
			float density = 1.0f;
			float elasticity = 1.0f;
			float friction = 0.5f;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(RigidbodyComponent2D, mass, elasticity, bodyType)
		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<physics::RigidbodyComponent2D>()
		{
			return "RigidbodyComponent2D";
		}
		template<>
		inline entt::hs GetTypeHashedString<physics::RigidbodyComponent2D>()
		{
			return entt::hs(GetTypeString<physics::RigidbodyComponent2D>().data());
		}

		template<>
		inline void RegisterType<physics::RigidbodyComponent2D>()
		{
			using namespace physics;

			auto meta = entt::meta<RigidbodyComponent2D>()
			.data<&RigidbodyComponent2D::bodyType>(entt::hs("bodyType"))
			.data<&RigidbodyComponent2D::mass>(entt::hs("mass"))
			.data<&RigidbodyComponent2D::density>(entt::hs("density"))
			.data<&RigidbodyComponent2D::elasticity>(entt::hs("elasticity"));

			reflection::RegisterTypeDefaults(meta);
			serialization::RegisterComponentSerializationTypeDefaults(meta);
		}
	}

	namespace serialization
	{
		template<>
		inline nlohmann::json Serialize<physics::RigidbodyComponent2D>(const physics::RigidbodyComponent2D& data)
		{
			nlohmann::json json;
			json["bodyType"] = physics::gBodyTypeToString.at(data.bodyType);
			json["mass"] = data.mass;
			json["density"] = data.density;
			json["elasticity"] = data.elasticity;
			json["friction"] = data.friction;
			return json;
		}

		template<>
		inline physics::RigidbodyComponent2D Deserialize<physics::RigidbodyComponent2D>(const nlohmann::json& json)
		{
			physics::RigidbodyComponent2D data;
			data.bodyType = physics::gStringToBodyType.at(json["bodyType"]);
			data.mass = json["mass"];
			data.density = json["density"];
			data.elasticity = json["elasticity"];
			data.friction = json["friction"];
			return data;
		}
	}
}
