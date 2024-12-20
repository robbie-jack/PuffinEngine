#pragma once

#include "nlohmann/json.hpp"

#include "puffin/components/physics/2d/shapecomponent2d.h"
#include "puffin/types/vector2.h"
#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"

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

	template<>
	inline void reflection::RegisterType<physics::BoxComponent2D>()
	{
		using namespace physics;

		entt::meta<BoxComponent2D>()
			.type(entt::hs("BoxComponent2D"))
			.data<&BoxComponent2D::centreOfMass>(entt::hs("centreOfMass"))
			.data<&BoxComponent2D::halfExtent>(entt::hs("halfExtent"));
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
