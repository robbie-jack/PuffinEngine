#pragma once

#include "nlohmann/json.hpp"

#include "puffin/types/vector3.h"
#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"

namespace puffin
{
	namespace physics
	{
		struct VelocityComponent3D
		{
#ifdef PFN_USE_DOUBLE_PRECISION
			Vector3d linear = Vector3d(0.0);
#else
			Vector3f linear = Vector3f(0.0f);
#endif

			Vector3f angular = Vector3(0.0f);

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(VelocityComponent3D, linear, angular)
		};
	}

	template<>
	inline void reflection::RegisterType<physics::VelocityComponent3D>()
	{
		using namespace physics;

		entt::meta<VelocityComponent3D>()
			.type(entt::hs("VelocityComponent3D"))
			.data<&VelocityComponent3D::linear>(entt::hs("linear"))
			.data<&VelocityComponent3D::angular>(entt::hs("angular"));
	}

	namespace serialization
	{
		template<>
		inline void Serialize<physics::VelocityComponent3D>(const physics::VelocityComponent3D& data, Archive& archive)
		{
			archive.Set("linear", data.linear);
			archive.Set("angular", data.angular);
		}

		template<>
		inline void Deserialize<physics::VelocityComponent3D>(const Archive& archive, physics::VelocityComponent3D& data)
		{
			archive.Get("linear", data.linear);
			archive.Get("angular", data.angular);
		}
	}
}