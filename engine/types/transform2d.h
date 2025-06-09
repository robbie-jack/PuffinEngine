#pragma once

#include "nlohmann/json.hpp"

#include "types/vector2.h"

namespace puffin
{
	struct Transform2D
	{
		Transform2D() = default;

#ifdef PFN_DOUBLE_PRECISION
		Transform2D(Vector2d position, float rotation = 0.0f, Vector2f scale = Vector2f(0.f, 0.f))
			: position(position), rotation(rotation), scale(scale) {}
#else
		Transform2D(Vector2f position, float rotation = 0.0f, Vector2f scale = Vector2f(0.f, 0.f))
			: position(position), rotation(rotation), scale(scale) {
		}
#endif

		Transform2D(const Transform2D& transform)
		{
			position = transform.position;
			rotation = transform.rotation;
			scale = transform.scale;
		}

		~Transform2D() = default;

		Transform2D& operator=(const Transform2D& transform)
		{
			position = transform.position;
			rotation = transform.rotation;
			scale = transform.scale;

			return *this;
		}

#ifdef PFN_DOUBLE_PRECISION
		Vector2d position = Vector2d(0.0);
#else
		Vector2f position;
#endif

		float rotation = 0.0f;

		Vector2f scale = Vector2f(0.f, 0.f);

	};

	namespace serialization
	{
		template<>
		inline nlohmann::json Serialize<Transform2D>(const Transform2D& data)
		{
			nlohmann::json json;
			json["position"] = Serialize(data.position);
			json["rotation"] = data.rotation;
			json["scale"] = Serialize(data.scale);
			return json;
		}

		template<>
		inline Transform2D Deserialize<Transform2D>(const nlohmann::json& json)
		{
			Transform2D data;
			data.position = Deserialize<Vector2f>(json["position"]);
			data.rotation = json["rotation"];
			data.scale = Deserialize<Vector2f>(json["scale"]);
			return data;
		}
	}
}
