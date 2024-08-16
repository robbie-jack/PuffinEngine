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

		explicit TransformNode2D(const std::shared_ptr<core::Engine>& engine, const UUID& id = gInvalidId);
		~TransformNode2D() override = default;

		void begin_play() override;
		void update(const double delta_time) override;
		void update_fixed(const double delta_time) override;
		void end_play() override;

		bool has_transform_2d() const override;
		const TransformComponent2D* transform_2d() const override;
		TransformComponent2D* transform_2d() override;

#ifdef PFN_DOUBLE_PRECISION
		[[nodiscard]] const Vector2d& position() const;
		[[nodiscard]] Vector2d& position();
		void set_position(const Vector2d& position);
#else
		[[nodiscard]] const Vector2f& position() const;
		[[nodiscard]] Vector2f& position();
		void set_position(const Vector2f& position);
#endif

		[[nodiscard]] const float& rotation() const;
		[[nodiscard]] float& rotation();
		void set_rotation(const float& rotation);

		[[nodiscard]] const Vector2f& scale() const;
		[[nodiscard]] Vector2f& scale();
		void set_scale(const Vector2f& scale);

	protected:

		

	};
}
