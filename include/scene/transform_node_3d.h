#pragma once

#include "scene/node.h"
#include "Types/Quat.h"
#include "Types/Vector.h"

namespace puffin
{
	struct TransformComponent3D;
}

namespace puffin::scene
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

		[[nodiscard]] const TransformComponent3D& get_transform() const;

#ifdef PFN_DOUBLE_PRECISION
		[[nodiscard]] const Vector3d& position() const;
		void set_position(const Vector3d& position) const;
#else
		[[nodiscard]] const Vector3f& position() const;
		void set_position(const Vector3f& position) const;
#endif

		[[nodiscard]] const maths::Quat& orientation() const;
		void set_orientation(const maths::Quat& orientation) const;

		[[nodiscard]] const Vector3f& scale() const;
		void set_scale(const Vector3f& scale) const;

	protected:



	};
}
