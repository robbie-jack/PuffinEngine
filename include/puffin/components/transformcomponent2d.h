#pragma once

#include "nlohmann/json.hpp"

#include "puffin/types/vector2.h"
#include "puffin/utility/reflection.h"

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

	template<>
	inline void reflection::RegisterType<TransformComponent2D>()
	{
		entt::meta<TransformComponent2D>()
			.type(entt::hs("TransformComponent2D"))
			.data<&TransformComponent2D::position>(entt::hs("position"))
			.data<&TransformComponent2D::rotation>(entt::hs("rotation"))
			.data<&TransformComponent2D::scale>(entt::hs("scale"));
	}

	namespace serialization
	{
		template<>
		inline void Serialize<TransformComponent2D>(const TransformComponent2D& data, Archive& archive)
		{
			archive.Set("position", data.position);
			archive.Set("rotation", data.rotation);
			archive.Set("scale", data.scale);
		}

		template<>
		inline void Deserialize<TransformComponent2D>(const Archive& archive, TransformComponent2D& data)
		{
			archive.Get("position", data.position);
			archive.Get("rotation", data.rotation);
			archive.Get("scale", data.scale);
		}
	}
}