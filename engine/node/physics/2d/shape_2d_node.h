#pragma once

#include "node/transform_2d_node.h"
#include "utility/reflection.h"

namespace puffin
{
	namespace physics
	{
		const std::string gShape2DNodeTypeString = "Shape2DNode";

		class Shape2DNode : public Transform2DNode
		{
		public:

			[[nodiscard]] std::string_view GetTypeString() const override;
			[[nodiscard]] entt::id_type GetTypeID() const override;

			const Vector2f& GetCentreOfMass() const;
			Vector2f& CentreOfMass();
			void SetCentreOfMass(const Vector2f& centreOfMass);

		private:

			Vector2f mCentreOfMass = { 0.0f };

		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<physics::Shape2DNode>()
		{
			return physics::gShape2DNodeTypeString;
		}

		template<>
		inline entt::hs GetTypeHashedString<physics::Shape2DNode>()
		{
			return entt::hs(GetTypeString<physics::Shape2DNode>().data());
		}

		template<>
		inline void RegisterType<physics::Shape2DNode>()
		{
			auto meta = entt::meta<physics::Shape2DNode>()
				.base<Transform2DNode>();

			reflection::RegisterTypeDefaults(meta);
		}
	}
}