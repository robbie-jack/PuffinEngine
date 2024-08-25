#pragma once

#include "puffin/nodes/node.h"
#include "puffin/types/vector2.h"
#include "puffin/utility/reflection.h"

namespace puffin
{
	struct TransformComponent2D;
}

namespace puffin
{
	const std::string gTransformNode2DTypeString = "TransformNode2D";
	const entt::id_type gTransformNode2DTypeID = entt::hs(gTransformNode2DTypeString.c_str());

	class TransformNode2D : public Node
	{
	public:

		void Initialize() override;
		void Deinitialize() override;

		[[nodiscard]] const std::string& GetTypeString() const override;
		[[nodiscard]] entt::id_type GetTypeID() const override;

		[[nodiscard]] const TransformComponent2D& GetTransform() const;
		[[nodiscard]] TransformComponent2D& Transform();

		[[nodiscard]] const TransformComponent2D& GetGlobalTransform() const;
		//[[nodiscard]] TransformComponent2D& GlobalTransform();

#ifdef PFN_DOUBLE_PRECISION
		[[nodiscard]] const Vector2d& GetPosition() const;
		[[nodiscard]] Vector2d& Position();
		void SetPosition(const Vector2d& position);
#else
		[[nodiscard]] const Vector2f& GetPosition() const;
		[[nodiscard]] Vector2f& Position();
		void SetPosition(const Vector2f& position);
#endif

		[[nodiscard]] const float& GetRotation() const;
		[[nodiscard]] float& Rotation();
		void SetRotation(const float& rotation);

		[[nodiscard]] const Vector2f& GetScale() const;
		[[nodiscard]] Vector2f& Scale();
		void SetScale(const Vector2f& scale);

	protected:

		

	};

	template<>
	inline void reflection::RegisterType<TransformNode2D>()
	{
		entt::meta<TransformNode2D>()
			.type(gTransformNode2DTypeID)
			.base<Node>()
			.custom<NodeCustomData>(gTransformNode2DTypeString);
	}
}
