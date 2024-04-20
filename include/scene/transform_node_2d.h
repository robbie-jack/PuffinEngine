#pragma once

#include "scene/node.h"
#include "Types/Vector.h"

namespace puffin
{
	struct TransformComponent2D;
}

namespace puffin::scene
{
	class TransformNode2D : public Node
	{
	public:

		explicit TransformNode2D(const std::shared_ptr<core::Engine>& engine, const PuffinID& id = gInvalidID);
		~TransformNode2D() override = default;

		void begin_play() override;
		void update(const double delta_time) override;
		void physics_update(const double delta_time) override;
		void end_play() override;

		const TransformComponent2D& get_transform() const;

#ifdef PFN_DOUBLE_PRECISION
		[[nodiscard]] const Vector2d& position() const;
		void set_position(const Vector2d& position) const;
#else
		[[nodiscard]] const Vector2f& position() const;
		void set_position(const Vector2f& position) const;
#endif

		[[nodiscard]] const float& rotation() const;
		void set_rotation(const float& rotation) const;

		[[nodiscard]] const Vector2f& scale() const;
		void set_scale(const Vector2f& scale) const;

	protected:

		

	};
}
