#pragma once

#include "nlohmann/json.hpp"

#include "component/physics/2d/shape_component_2d.h"
#include "types/vector2.h"
#include "utility/reflection.h"
#include "utility/serialization.h"
#include "serialization/component_serialization.h"

namespace puffin
{
	namespace physics
	{
		struct BoxComponent2D : ShapeComponent2D
		{
			BoxComponent2D() = default;

			explicit BoxComponent2D(const Vector2f& halfExtent) : halfExtent(halfExtent) {}

			Vector2f halfExtent = { 0.5f };

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(BoxComponent2D, centreOfMass, halfExtent)
		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<physics::BoxComponent2D>()
		{
			return "BoxComponent2D";
		}
		template<>
		inline entt::hs GetTypeHashedString<physics::BoxComponent2D>()
		{
			return entt::hs(GetTypeString<physics::BoxComponent2D>().data());
		}


		template<>
		inline void RegisterType<physics::BoxComponent2D>()
		{
			using namespace physics;

			auto meta = entt::meta<BoxComponent2D>()
			.data<&BoxComponent2D::centreOfMass>(entt::hs("centreOfMass"))
			.data<&BoxComponent2D::halfExtent>(entt::hs("halfExtent"));

			reflection::RegisterTypeDefaults(meta);
			serialization::RegisterComponentSerializationTypeDefaults(meta);
		}
	}

	namespace serialization
	{
		template<>
		inline nlohmann::json Serialize<physics::BoxComponent2D>(const physics::BoxComponent2D& data)
		{
			nlohmann::json json;
			json["centreOfMass"] = Serialize(data.centreOfMass);
			json["halfExtent"] = Serialize(data.halfExtent);
			return json;
		}

		template<>
		inline physics::BoxComponent2D Deserialize<physics::BoxComponent2D>(const nlohmann::json& json)
		{
			physics::BoxComponent2D data;
			data.centreOfMass = Deserialize<Vector2f>(json["centreOfMass"]);
			data.halfExtent = Deserialize<Vector2f>(json["halfExtent"]);
			return data;
		}
	}
}
