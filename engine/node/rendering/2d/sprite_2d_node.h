#pragma once

#include "node/transform_2d_node.h"
#include "types/vector3.h"
#include "utility/reflection.h"

namespace puffin
{
	namespace rendering
	{
		const std::string gSprite2DNodeTypeString = "Sprite2DNode";

		class Sprite2DNode : public Transform2DNode
		{
		public:

			void Initialize() override;
			void Deinitialize() override;

			void Serialize(nlohmann::json& json) const override;
			void Deserialize(const nlohmann::json& json) override;

			[[nodiscard]] std::string_view GetTypeString() const override;
			[[nodiscard]] entt::id_type GetTypeID() const override;

			[[nodiscard]] const Vector3f& GetColour() const;
			[[nodiscard]] Vector3f& Colour();
			void SetColour(const Vector3f colour);

			[[nodiscard]] const Vector2f& GetOffset() const;
			[[nodiscard]] Vector2f& Offset();
			void SetOffset(const Vector2f offset);

		private:

			Vector3f mColour = { 1.f, 1.f, 1.f };
			Vector2f mOffset = { 0.f, 0.f };

		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<rendering::Sprite2DNode>()
		{
			return rendering::gSprite2DNodeTypeString;
		}

		template<>
		inline entt::hs GetTypeHashedString<rendering::Sprite2DNode>()
		{
			return entt::hs(GetTypeString<rendering::Sprite2DNode>().data());
		}

		template<>
		inline void RegisterType<rendering::Sprite2DNode>()
		{
			auto meta = entt::meta<rendering::Sprite2DNode>()
			.base<Transform2DNode>();

			reflection::RegisterTypeDefaults(meta);
		}
	}
}