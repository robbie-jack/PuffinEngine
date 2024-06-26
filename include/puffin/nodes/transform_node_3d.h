#pragma once

#include "puffin/nodes/node.h"
#include "puffin/types/quat.h"
#include "puffin/types/euler_angles.h"
#include "puffin/types/vector.h"

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
		void update_fixed(const double delta_time) override;
		void end_play() override;

		bool has_transform_3d() const override;
		const TransformComponent3D* transform_3d() const override;
		TransformComponent3D* transform_3d() override;

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

		[[nodiscard]] const maths::EulerAngles& euler_angles() const;
		[[nodiscard]] maths::EulerAngles& euler_angles();
		void set_euler_angles(const maths::EulerAngles& euler_angles);

		[[nodiscard]] const Vector3f& scale() const;
		[[nodiscard]] Vector3f& scale();
		void set_scale(const Vector3f& scale);

	protected:

	};
}
