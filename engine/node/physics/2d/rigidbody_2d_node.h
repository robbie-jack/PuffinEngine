#pragma once

#include "node/transform_2d_node.h"
#include "physics/body_type.h"

namespace puffin
{
	namespace physics
	{
		const std::string gRigidbody2DNodeTypeString = "Rigidbody2DNode";

		class Rigidbody2DNode : public Transform2DNode
		{
		public:

			Rigidbody2DNode() = default;
			~Rigidbody2DNode() override = default;

			void Serialize(nlohmann::json& json) const override;
			void Deserialize(const nlohmann::json& json) override;

			[[nodiscard]] std::string_view GetTypeString() const override;
			[[nodiscard]] entt::id_type GetTypeID() const override;

			BodyType GetBodyType() const;
			BodyType& BodyType();
			void SetBodyType(enum BodyType bodyType);

			float GetMass() const;
			float& Mass();
			void SetMass(float mass);

			float GetDensity() const;
			float& Density();
			void SetDensity(float density);

			float GetElasticity() const;
			float& Elasticity();
			void SetElasticity(float elasticity);

			float GetFriction() const;
			float& Friction();
			void SetFriction(float friction);

			Vector2f GetLinearVelocity() const;
			Vector2f& GetLinearVelocity();
			void SetLinearVelocity(Vector2f linearVelocity);

			float GetAngularVelocity() const;
			float& AngularVelocity();
			void SetAngularVelocity(float angularVelocity);

		private:

			enum BodyType mBodyType = BodyType::Static;

			float mMass = 0.0f;
			float mDensity = 1.0f;
			float mElasticity = 1.0f;
			float mFriction = 0.5f;

			Vector2f mLinearVelocity = { 0.0f };
			float mAngularVelocity = 0.0f;

		};

	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<physics::Rigidbody2DNode>()
		{
			return physics::gRigidbody2DNodeTypeString;
		}

		template<>
		inline entt::hs GetTypeHashedString<physics::Rigidbody2DNode>()
		{
			return entt::hs(GetTypeString<physics::Rigidbody2DNode>().data());
		}

		template<>
		inline void RegisterType<physics::Rigidbody2DNode>()
		{
			auto meta = entt::meta<physics::Rigidbody2DNode>()
				.base<Transform2DNode>();

			reflection::RegisterTypeDefaults(meta);
		}
	}
}