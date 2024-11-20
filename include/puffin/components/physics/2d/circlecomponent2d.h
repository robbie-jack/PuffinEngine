#pragma once

#include "nlohmann/json.hpp"

#include "puffin/components/physics/2d/shapecomponent2d.h"
#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"

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

	template<>
	inline void reflection::RegisterType<physics::CircleComponent2D>()
	{
		using namespace physics;

		entt::meta<CircleComponent2D>()
			.type(entt::hs("CircleComponent2D"))
			.data<&CircleComponent2D::centreOfMass>(entt::hs("centreOfMass"))
			.data<&CircleComponent2D::radius>(entt::hs("radius"));
	}

	namespace serialization
	{
		template<>
		inline void Serialize<physics::CircleComponent2D>(const physics::CircleComponent2D& data, Archive& archive)
		{
			archive.Set("centreOfMass", data.centreOfMass);
			archive.Set("radius", data.radius);
		}

		template<>
		inline void Deserialize<physics::CircleComponent2D>(const Archive& archive, physics::CircleComponent2D& data)
		{
			archive.Get("centreOfMass", data.centreOfMass);
			archive.Get("radius", data.radius);
		}

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