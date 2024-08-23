#pragma once

#include "nlohmann/json.hpp"

#include "puffin/components/physics/3d/shapecomponent3d.h"
#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"

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
		inline void Serialize<physics::SphereComponent3D>(const physics::SphereComponent3D& data, Archive& archive)
		{
			archive.Set("centreOfMass", data.centreOfMass);
			archive.Set("radius", data.radius);
		}

		template<>
		inline void Deserialize<physics::SphereComponent3D>(const Archive& archive, physics::SphereComponent3D& data)
		{
			archive.Get("centreOfMass", data.centreOfMass);
			archive.Get("radius", data.radius);
		}
	}
}