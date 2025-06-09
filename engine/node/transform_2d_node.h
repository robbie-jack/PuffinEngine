#pragma once

#include "node/node.h"
#include "types/vector2.h"
#include "types/transform2d.h"
#include "utility/reflection.h"

namespace puffin
{
	struct TransformComponent2D;
}

namespace puffin
{
	const std::string gTransformNode2DTypeString = "Transform2DNode";

	class Transform2DNode : public Node
	{
	public:

		void Initialize() override;
		void Deinitialize() override;

		void Update(double deltaTime) override;

		const std::string& GetTypeString() const override;
		entt::id_type GetTypeID() const override;

		const Transform2D& GetTransform() const;
		Transform2D& Transform();
		void SetTransform(const Transform2D& transform);

#ifdef PFN_DOUBLE_PRECISION
		const Vector2d& GetPosition() const;
		Vector2d& Position();
		void SetPosition(const Vector2d& position);
#else
		const Vector2f& GetPosition() const;
		Vector2f& Position();
		void SetPosition(const Vector2f& position);
#endif

		const float& GetRotation() const;
		float& Rotation();
		void SetRotation(const float& rotation);

		const Vector2f& GetScale() const;
		Vector2f& Scale();
		void SetScale(const Vector2f& scale);

		const Transform2D& GetGlobalTransform() const;
		Transform2D& GlobalTransform();
		void SetGlobalTransform(const Transform2D& transform);

	protected:



	private:

		void UpdateLocalTransform();
		void UpdateGlobalTransform();

		/*
		 * Local space transform
		 */
		Transform2D mLocalTransform = Transform2D();

		/*
		 * Global space transform
		 */
		Transform2D mGlobalTransform = Transform2D();

		

	};

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<Transform2DNode>()
		{
			return "Transform2DNode";
		}

		template<>
		inline entt::hs GetTypeHashedString<Transform2DNode>()
		{
			return entt::hs(GetTypeString<Transform2DNode>().data());
		}

		template<>
		inline void RegisterType<Transform2DNode>()
		{
			auto meta = entt::meta<Transform2DNode>()
			.base<Node>();

			reflection::RegisterTypeDefaults(meta);
		}
	}
}
