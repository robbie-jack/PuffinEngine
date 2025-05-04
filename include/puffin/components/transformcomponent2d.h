#pragma once

#include "nlohmann/json.hpp"

#include "puffin/types/vector2.h"
#include "puffin/utility/reflection.h"
#include "puffin/serialization/componentserialization.h"

namespace puffin
{
	struct TransformComponent2D
	{
		TransformComponent2D() = default;

#ifdef PFN_DOUBLE_PRECISION
		TransformComponent2D(const Vector2d& position) : position(position) {}

		TransformComponent2D(const Vector2d& position, const float& rotation, const Vector2f& scale) :
			position(position), rotation(rotation), scale(scale) {}
#else
		explicit TransformComponent2D(const Vector2f& position) : position(position) {}

		TransformComponent2D(const Vector2f& position, const float& rotation, const Vector2f& scale) :
			position(position), rotation(rotation), scale(scale) {}
#endif

		TransformComponent2D(const TransformComponent2D& t) = default;

		~TransformComponent2D() = default;

		TransformComponent2D& operator=(const TransformComponent2D& rhs) = default;

#ifdef PFN_DOUBLE_PRECISION
		Vector2d position = Vector2d(0.0);
#else
		Vector2f position = Vector2f(0.0f);
#endif

		float rotation = 0.0;

		Vector2f scale = Vector2f(1.0f);

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(TransformComponent2D, position, rotation, scale)
	};

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<TransformComponent2D>()
		{
			return "TransformComponent2D";
		}

		template<>
		inline entt::hs GetTypeHashedString<TransformComponent2D>()
		{
			return entt::hs(GetTypeString<TransformComponent2D>().data());
		}

		template<>
		inline void RegisterType<TransformComponent2D>()
		{
			entt::meta<TransformComponent2D>()
				.type(GetTypeHashedString<TransformComponent2D>())
				.func<&GetTypeString<TransformComponent2D>>(entt::hs("GetTypeString"))
				.func<&GetTypeHashedString<TransformComponent2D>>(entt::hs("GetTypeHashedString"))
				.func<&serialization::HasComponent<TransformComponent2D>>(entt::hs("HasComponent"))
				.func<&serialization::SerializeFromRegistry<TransformComponent2D>>(entt::hs("SerializeFromRegistry"))
				.func<&serialization::DeserializeToRegistry<TransformComponent2D>>(entt::hs("DeserializeToRegistry"))
				.data<&TransformComponent2D::position>(entt::hs("position"))
				.data<&TransformComponent2D::rotation>(entt::hs("rotation"))
				.data<&TransformComponent2D::scale>(entt::hs("scale"));

			auto* registry = serialization::ComponentRegistry::Get();
			registry->Register<TransformComponent2D>();
		}
	}

	namespace serialization
	{
		template<>
		inline nlohmann::json Serialize<TransformComponent2D>(const TransformComponent2D& data)
		{
			nlohmann::json json;
			json["position"] = Serialize(data.position);
			json["rotation"] = data.rotation;
			json["scale"] = Serialize(data.scale);
			return json;
		}

		template<>
		inline TransformComponent2D Deserialize<TransformComponent2D>(const nlohmann::json& json)
		{
			TransformComponent2D data;
			data.position = Deserialize<Vector2f>(json["position"]);
			data.rotation = json["rotation"];
			data.scale = Deserialize<Vector2f>(json["scale"]);
			return data;
		}
	}
}