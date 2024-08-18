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

		[[nodiscard]] const TransformComponent3D& GetTransform() const;
		[[nodiscard]] TransformComponent3D& Transform();

		[[nodiscard]] const TransformComponent3D& GetGlobalTransform() const;
		//[[nodiscard]] TransformComponent3D& GlobalTransform();

#ifdef PFN_DOUBLE_PRECISION
		[[nodiscard]] const Vector3d& GetPosition() const;
		[[nodiscard]] Vector3d& Position();
		void SetPosition(const Vector3d& position);
#else
		[[nodiscard]] const Vector3f& GetPosition() const;
		[[nodiscard]] Vector3f& Position();
		void SetPosition(const Vector3f& position);
#endif

		[[nodiscard]] const maths::Quat& GetOrientationQuat() const;
		[[nodiscard]] maths::Quat& OrientationQuat();
		void SetOrientationQuat(const maths::Quat& orientation);

		[[nodiscard]] const maths::EulerAngles& GetOrientationEulerAngles() const;
		[[nodiscard]] maths::EulerAngles& OrientationEulerAngles();
		void SetOrientationEulerAngles(const maths::EulerAngles& eulerAngles);

		[[nodiscard]] const Vector3f& SetScale() const;
		[[nodiscard]] Vector3f& Scale();
		void SetScale(const Vector3f& scale);

	protected:

	};
}
