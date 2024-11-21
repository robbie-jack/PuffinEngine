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
		inline nlohmann::json Serialize<physics::VelocityComponent3D>(const physics::VelocityComponent3D& data)
		{
			nlohmann::json json;
			json["linear"] = Serialize(data.linear);
			json["angular"] = Serialize(data.angular);
			return json;
		}

		template<>
		inline physics::VelocityComponent3D Deserialize<physics::VelocityComponent3D>(const nlohmann::json& json)
		{
			physics::VelocityComponent3D data;
			data.linear = Deserialize<Vector3f>(json["linear"]);
			data.angular = Deserialize<Vector3f>(json["angular"]);
			return data;
		}
	}
}