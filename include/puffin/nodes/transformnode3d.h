#pragma once

#include "puffin/nodes/node.h"
#include "puffin/types/quat.h"
#include "puffin/types/eulerangles.h"
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

		explicit TransformNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id = gInvalidID);
		~TransformNode3D() override = default;

		void BeginPlay() override;
		void Update(const double delta_time) override;
		void FixedUpdate(const double delta_time) override;
		void EndPlay() override;

		bool HasTransform3D() const override;
		const TransformComponent3D* GetTransform3D() const override;
		TransformComponent3D* GetTransform3D() override;

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
