#pragma once

#include "nlohmann/json.hpp"

#include "component/physics/3d/shape_component_3d.h"
#include "utility/reflection.h"
#include "utility/serialization.h"

namespace puffin
{
	namespace physics
	{
		struct SphereComponent3D : ShapeComponent3D
		{
			SphereComponent3D() = default;

			explicit SphereComponent3D(const float& radius) : radius(radius) {}

			float radius = 0.5f;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(SphereComponent3D, centreOfMass, radius)
		};
	}

	template<>
	inline void reflection::RegisterType<physics::SphereComponent3D>()
	{
		using namespace physics;

		entt::meta<SphereComponent3D>()
			.type(entt::hs("SphereComponent3D"))
			.data<&SphereComponent3D::centreOfMass>(entt::hs("centreOfMass"))
			.data<&SphereComponent3D::radius>(entt::hs("radius"));
	}

	namespace serialization
	{
		template<>
		inline nlohmann::json Serialize<physics::SphereComponent3D>(const physics::SphereComponent3D& data)
		{
			nlohmann::json json;
			json["centreOfMass"] = Serialize(data.centreOfMass);
			json["halfExtent"] = data.radius;
			return json;
		}

		template<>
		inline physics::SphereComponent3D Deserialize<physics::SphereComponent3D>(const nlohmann::json& json)
		{
			physics::SphereComponent3D data;
			data.centreOfMass = Deserialize<Vector3f>(json["centreOfMass"]);
			data.radius = json["radius"];
			return data;
		}
	}
}