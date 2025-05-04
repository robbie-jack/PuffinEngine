#pragma once

#include "nlohmann/json.hpp"

#include "puffin/types/vector2.h"
#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"
#include "puffin/serialization/componentserialization.h"

namespace puffin
{
	namespace physics
	{
		struct VelocityComponent2D
		{
#ifdef PFN_USE_DOUBLE_PRECISION
			Vector2d linear = Vector2d(0.0);
#else
			Vector2f linear = Vector2f(0.0f);
#endif

			float angular = 0.0f;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(VelocityComponent2D, linear, angular)
		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<physics::VelocityComponent2D>()
		{
			return "VelocityComponent2D";
		}
		template<>
		inline entt::hs GetTypeHashedString<physics::VelocityComponent2D>()
		{
			return entt::hs(GetTypeString<physics::VelocityComponent2D>().data());
		}

		template<>
		inline void RegisterType<physics::VelocityComponent2D>()
		{
			using namespace physics;

			auto meta = entt::meta<VelocityComponent2D>()
			.data<&VelocityComponent2D::linear>(entt::hs("linear"))
			.data<&VelocityComponent2D::angular>(entt::hs("angular"));

			reflection::RegisterTypeDefaults(meta);
			serialization::RegisterComponentSerializationTypeDefaults(meta);
		}
	}

	namespace serialization
	{
		template<>
		inline nlohmann::json Serialize<physics::VelocityComponent2D>(const physics::VelocityComponent2D& data)
		{
			nlohmann::json json;
			json["linear"] = Serialize(data.linear);
			json["angular"] = data.angular;
			return json;
		}

		template<>
		inline physics::VelocityComponent2D Deserialize<physics::VelocityComponent2D>(const nlohmann::json& json)
		{
			physics::VelocityComponent2D data;
			data.linear = Deserialize<Vector2f>(json["linear"]);
			data.angular = json["angular"];
			return data;
		}
	}
}