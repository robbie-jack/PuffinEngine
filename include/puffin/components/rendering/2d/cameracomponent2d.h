#pragma once

#include "nlohmann/json.hpp"

#include "puffin/types/vector2.h"
#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"

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
			float zoom = 0.0f;
		};
	}

	template<>
	inline void reflection::RegisterType<rendering::CameraComponent2D>()
	{
		entt::meta<rendering::CameraComponent2D>()
			.type(entt::hs("CameraComponent2D"))
			.data<&rendering::CameraComponent2D::active>(entt::hs("active"))
			.data<&rendering::CameraComponent2D::offset>(entt::hs("offset"))
			.data<&rendering::CameraComponent2D::rotation>(entt::hs("rotation"))
			.data<&rendering::CameraComponent2D::zoom>(entt::hs("zoom"));
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