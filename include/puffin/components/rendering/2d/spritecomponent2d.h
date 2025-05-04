#pragma once

#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"

namespace puffin
{
	namespace rendering
	{
		struct SpriteComponent2D
		{

		};
	}

	template<>
	inline void reflection::RegisterType<rendering::SpriteComponent2D>()
	{
		entt::meta<rendering::SpriteComponent2D>()
			.type(entt::hs("SpriteComponent2D"));
	}

	namespace serialization
	{
		template<>
		inline nlohmann::json Serialize<rendering::SpriteComponent2D>(const rendering::SpriteComponent2D& data)
		{
			nlohmann::json json;

			return json;
		}

		template<>
		inline rendering::SpriteComponent2D Deserialize<rendering::SpriteComponent2D>(const nlohmann::json& json)
		{
			rendering::SpriteComponent2D data;

			return data;
		}
	}
}