#pragma once

#include "node/transform_2d_node.h"
#include "types/vector3.h"
#include "utility/reflection.h"

namespace puffin
{
	namespace rendering
	{
		const std::string gSpriteNode2DTypeString = "SpriteNode2D";

		class SpriteNode2D : public TransformNode2D
		{
		public:

			void Initialize() override;
			void Deinitialize() override;

			[[nodiscard]] const std::string& GetTypeString() const override;
			[[nodiscard]] entt::id_type GetTypeID() const override;

			[[nodiscard]] const Vector3f& GetColour() const;
			[[nodiscard]] Vector3f& Colour();
			void SetColour(const Vector3f colour);

			[[nodiscard]] const Vector2f& GetOffset() const;
			[[nodiscard]] Vector2f& Offset();
			void SetOffset(const Vector2f offset);
		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<rendering::SpriteNode2D>()
		{
			return rendering::gSpriteNode2DTypeString;
		}

		template<>
		inline entt::hs GetTypeHashedString<rendering::SpriteNode2D>()
		{
			return entt::hs(GetTypeString<rendering::SpriteNode2D>().data());
		}

		template<>
		inline void RegisterType<rendering::SpriteNode2D>()
		{
			auto meta = entt::meta<rendering::SpriteNode2D>()
			.base<TransformNode2D>();

			reflection::RegisterTypeDefaults(meta);
		}
	}
}