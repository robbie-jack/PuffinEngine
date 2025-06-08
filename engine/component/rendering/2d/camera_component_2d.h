#pragma once

#include "nlohmann/json.hpp"

#include "types/vector2.h"
#include "utility/reflection.h"
#include "utility/serialization.h"
#include "serialization/component_serialization.h"

namespace puffin
{
	namespace rendering
	{
		struct CameraComponent2D
		{
			CameraComponent2D() = default;

			bool active = false;
			Vector2f offset;
			float rotation = 0.0f;
			float zoom = 1.0f;
		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<rendering::CameraComponent2D>()
		{
			return "CameraComponent2D";
		}
		template<>
		inline entt::hs GetTypeHashedString<rendering::CameraComponent2D>()
		{
			return entt::hs(GetTypeString<rendering::CameraComponent2D>().data());
		}

		template<>
		inline void RegisterType<rendering::CameraComponent2D>()
		{
			using namespace rendering;

			auto meta = entt::meta<CameraComponent2D>()
			.data<&CameraComponent2D::active>(entt::hs("active"))
			.data<&CameraComponent2D::offset>(entt::hs("offset"))
			.data<&CameraComponent2D::rotation>(entt::hs("rotation"))
			.data<&CameraComponent2D::zoom>(entt::hs("zoom"));

			reflection::RegisterTypeDefaults(meta);
			serialization::RegisterComponentSerializationTypeDefaults(meta);
		}
	}

	namespace serialization
	{
		template<>
		inline nlohmann::json Serialize<rendering::CameraComponent2D>(const rendering::CameraComponent2D& data)
		{
			nlohmann::json json;
			json["active"] = data.active;
			json["offset"] = Serialize(data.offset);
			json["rotation"] = data.rotation;
			json["zoom"] = data.zoom;
			return json;
		}

		template<>
		inline rendering::CameraComponent2D Deserialize<rendering::CameraComponent2D>(const nlohmann::json& json)
		{
			rendering::CameraComponent2D data;
			data.active = json["active"];
			data.offset = Deserialize<Vector2f>(json["offset"]);
			data.rotation = json["rotation"];
			data.zoom = json["zoom"];
			return data;
		}
	}
}