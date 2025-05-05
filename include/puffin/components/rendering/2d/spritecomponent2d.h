#pragma once

#include "nlohmann/json.hpp"

#include "puffin/types/vector3.h"
#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"
#include "puffin/serialization/componentserialization.h"

namespace puffin
{
	namespace rendering
	{
		struct SpriteComponent2D
		{
			Vector3f colour = {1.f, 1.f, 1.f};
			Vector2f offset = {0.f, 0.f};

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(SpriteComponent2D, colour, offset)
		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<rendering::SpriteComponent2D>()
		{
			return "SpriteComponent2D";
		}
		template<>
		inline entt::hs GetTypeHashedString<rendering::SpriteComponent2D>()
		{
			return entt::hs(GetTypeString<rendering::SpriteComponent2D>().data());
		}

		template<>
		inline void RegisterType<rendering::SpriteComponent2D>()
		{
			using namespace rendering;

			auto meta = entt::meta<SpriteComponent2D>()
			.data<&SpriteComponent2D::colour>(entt::hs("colour"))
			.data<&SpriteComponent2D::offset>(entt::hs("offset"));

			reflection::RegisterTypeDefaults(meta);
			serialization::RegisterComponentSerializationTypeDefaults(meta);
		}
	}

	namespace serialization
	{
		template<>
		inline nlohmann::json Serialize<rendering::SpriteComponent2D>(const rendering::SpriteComponent2D& data)
		{
			nlohmann::json json;
			json["colour"] = Serialize(data.colour);
			json["offset"] = Serialize(data.offset);
			return json;
		}

		template<>
		inline rendering::SpriteComponent2D Deserialize<rendering::SpriteComponent2D>(const nlohmann::json& json)
		{
			rendering::SpriteComponent2D data;
			data.colour = Deserialize<Vector3f>(json["colour"]);
			data.offset = Deserialize<Vector2f>(json["offset"]);
			return data;
		}
	}
}