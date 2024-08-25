#pragma once

#include "puffin/nodes/node.h"
#include "puffin/types/quat.h"
#include "puffin/types/eulerangles.h"
#include "puffin/types/vector3.h"
#include "puffin/utility/reflection.h"

namespace puffin
{
	struct TransformComponent3D;
}

namespace puffin
{
	const std::string gTransformNode3DTypeString = "TransformNode3D";
	const entt::id_type gTransformNode3DTypeID = entt::hs(gTransformNode3DTypeString.c_str());

	class TransformNode3D : public Node
	{
	public:

		void Initialize() override;
		void Deinitialize() override;

		[[nodiscard]] const std::string& GetTypeString() const override;
		[[nodiscard]] entt::id_type GetTypeID() const override;

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

	template<>
	inline void reflection::RegisterType<TransformNode3D>()
	{
		entt::meta<TransformNode3D>()
			.type(gTransformNode3DTypeID)
			.base<Node>()
			.custom<NodeCustomData>(gTransformNode3DTypeString);
	}
}
