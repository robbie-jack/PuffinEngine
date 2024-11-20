#pragma once

#include "nlohmann/json.hpp"

#include "puffin/components/physics/3d/shapecomponent3d.h"
#include "puffin/types/vector3.h"
#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"

namespace puffin
{
	namespace physics
	{	
		struct BoxComponent3D : ShapeComponent3D
		{
			BoxComponent3D() = default;

			explicit BoxComponent3D(const Vector3f& halfExtent) : halfExtent(halfExtent) {}

			Vector3f halfExtent = { 0.5f };

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(BoxComponent3D, centreOfMass, halfExtent)
		};
	}

	template<>
	inline void reflection::RegisterType<physics::BoxComponent3D>()
	{
		using namespace physics;

		entt::meta<BoxComponent3D>()
			.type(entt::hs("BoxComponent3D"))
			.data<&BoxComponent3D::centreOfMass>(entt::hs("centreOfMass"))
			.data<&BoxComponent3D::halfExtent>(entt::hs("halfExtent"));
	}

	namespace serialization
	{
		template<>
		inline void Serialize<physics::BoxComponent3D>(const physics::BoxComponent3D& data, Archive& archive)
		{
			archive.Set("centreOfMass", data.centreOfMass);
			archive.Set("halfExtent", data.halfExtent);
		}

		template<>
		inline void Deserialize<physics::BoxComponent3D>(const Archive& archive, physics::BoxComponent3D& data)
		{
			archive.Get("centreOfMass", data.centreOfMass);
			archive.Get("halfExtent", data.halfExtent);
		}

		template<>
		inline nlohmann::json Serialize<physics::BoxComponent3D>(const physics::BoxComponent3D& data)
		{
			nlohmann::json json;
			json["centreOfMass"] = Serialize(data.centreOfMass);
			json["halfExtent"] = Serialize(data.halfExtent);
			return json;
		}

		template<>
		inline physics::BoxComponent3D Deserialize<physics::BoxComponent3D>(const nlohmann::json& json)
		{
			physics::BoxComponent3D data;
			data.centreOfMass = Deserialize<Vector3f>(json["centreOfMass"]);
			data.halfExtent = Deserialize<Vector3f>(json["halfExtent"]);
			return data;
		}
	}
}
