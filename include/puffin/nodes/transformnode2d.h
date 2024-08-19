#pragma once

#include "puffin/nodes/node.h"
#include "puffin/types/vector.h"

namespace puffin
{
	struct TransformComponent2D;
}

namespace puffin
{
	class TransformNode2D : public Node
	{
	public:

		explicit TransformNode2D(const std::shared_ptr<core::Engine>& engine, const UUID& id = gInvalidID);
		~TransformNode2D() override = default;

		void Initialize() override;
		void Deinitialize() override;

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
}
