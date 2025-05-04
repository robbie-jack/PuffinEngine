#pragma once

#include "nlohmann/json.hpp"

#include "puffin/components/physics/2d/shapecomponent2d.h"
#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"
#include "puffin/serialization/componentserialization.h"

namespace puffin
{
	namespace physics
	{
		struct CircleComponent2D : ShapeComponent2D
		{
			CircleComponent2D() = default;

			explicit CircleComponent2D(const float& radius) : radius(radius) {}

			float radius = 0.5f;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(CircleComponent2D, centreOfMass, radius)
		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<physics::CircleComponent2D>()
		{
			return "CircleComponent2D";
		}

		template<>
		inline entt::hs GetTypeHashedString<physics::CircleComponent2D>()
		{
			return entt::hs(GetTypeString<physics::CircleComponent2D>().data());
		}

		template<>
		inline void RegisterType<physics::CircleComponent2D>()
		{
			using namespace physics;

			auto meta = entt::meta<CircleComponent2D>()
			.data<&CircleComponent2D::centreOfMass>(entt::hs("centreOfMass"))
			.data<&CircleComponent2D::radius>(entt::hs("radius"));

			reflection::RegisterTypeDefaults(meta);
			serialization::RegisterComponentSerializationTypeDefaults(meta);
		}
	}

	namespace serialization
	{
		template<>
		inline nlohmann::json Serialize<physics::CircleComponent2D>(const physics::CircleComponent2D& data)
		{
			nlohmann::json json;
			json["centreOfMass"] = Serialize(data.centreOfMass);
			json["radius"] = data.radius;
			return json;
		}

		template<>
		inline physics::CircleComponent2D Deserialize<physics::CircleComponent2D>(const nlohmann::json& json)
		{
			physics::CircleComponent2D data;
			data.centreOfMass = Deserialize<Vector2f>(json["centreOfMass"]);
			data.radius = json["radius"];
			return data;
		}
	}
}