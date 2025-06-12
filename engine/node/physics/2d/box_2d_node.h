#pragma once

#include "node/physics/2d/shape_2d_node.h"

namespace puffin
{
	namespace physics
	{
		const std::string gBox2DNodeTypeString = "Box2DNode";

		class Box2DNode : public Shape2DNode
		{
		public:

			void Serialize(nlohmann::json& json) const override;
			void Deserialize(const nlohmann::json& json) override;

			[[nodiscard]] std::string_view GetTypeString() const override;
			[[nodiscard]] entt::id_type GetTypeID() const override;

			const Vector2f& GetHalfExtent() const;
			Vector2f& HalfExtent();
			void SetHalfExtent(const Vector2f& halfExtent);

		private:

			Vector2f mHalfExtent = { 0.5f };

		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<physics::Box2DNode>()
		{
			return physics::gBox2DNodeTypeString;
		}

		template<>
		inline entt::hs GetTypeHashedString<physics::Box2DNode>()
		{
			return entt::hs(GetTypeString<physics::Box2DNode>().data());
		}

		template<>
		inline void RegisterType<physics::Box2DNode>()
		{
			auto meta = entt::meta<physics::Box2DNode>()
				.base<physics::Shape2DNode>();

			reflection::RegisterTypeDefaults(meta);
		}
	}
}