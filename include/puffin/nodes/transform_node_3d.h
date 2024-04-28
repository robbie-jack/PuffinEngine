#pragma once

#include "puffin/nodes/node.h"
#include "puffin/types/Quat.h"
#include "puffin/types/Vector.h"

namespace puffin
{
	struct TransformComponent3D;
}

namespace puffin
{
	class TransformNode3D : public Node
	{
	public:

		explicit TransformNode3D(const std::shared_ptr<core::Engine>& engine, const PuffinID& id = gInvalidID);
		~TransformNode3D() override = default;

		void begin_play() override;
		void update(const double delta_time) override;
		void physics_update(const double delta_time) override;
		void end_play() override;

		bool has_transform_3d() const override;
		const TransformComponent3D& transform_3d() const override;
		TransformComponent3D& transform_3d() override;

		[[nodiscard]] const TransformComponent3D& transform() const;
		[[nodiscard]] TransformComponent3D& transform();

#ifdef PFN_DOUBLE_PRECISION
		[[nodiscard]] const Vector3d& position() const;
		[[nodiscard]] Vector3d& position();
		void set_position(const Vector3d& position);
#else
		[[nodiscard]] const Vector3f& position() const;
		[[nodiscard]] Vector3f& position();
		void set_position(const Vector3f& position);
#endif

		[[nodiscard]] const maths::Quat& orientation() const;
		[[nodiscard]] maths::Quat& orientation();
		void set_orientation(const maths::Quat& orientation);

		[[nodiscard]] const Vector3f& scale() const;
		[[nodiscard]] Vector3f& scale();
		void set_scale(const Vector3f& scale);

	protected:



	};
}
