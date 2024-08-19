#pragma once

#include "puffin/types/vector.h"
#include "puffin/types/quat.h"

#include "nlohmann/json.hpp"
#include "puffin/types/eulerangles.h"

#include "puffin/utility/reflection.h"

namespace puffin
{
//#ifdef PFN_DOUBLE_PRECISION
//	constexpr bool gUseDoublePrecision = true;
//#else
//	constexpr bool gUseDoublePrecision = false;
//#endif

	struct TransformComponent3D
	{
		TransformComponent3D() = default;

#ifdef PFN_DOUBLE_PRECISION
		TransformComponent3D(const Vector3d& position) : position(position) {}

		TransformComponent3D(const Vector3d& position, const maths::Quat& orientation, const maths::EulerAngles& euler, const Vector3f& scale) :
			position(position), orientation(orientation), orientation_euler_angles(euler), scale(scale) {}
#else
		explicit TransformComponent3D(const Vector3f& position) : position(position) {}

		TransformComponent3D(const Vector3f& position, const maths::Quat& orientation, const maths::EulerAngles& euler, const Vector3f& scale) :
			position(position), orientationQuat(orientation), orientationEulerAngles(euler), scale(scale) {}
#endif

		TransformComponent3D(const TransformComponent3D& t) = default;

		~TransformComponent3D() = default;

		TransformComponent3D& operator=(const TransformComponent3D& rhs) = default;

#ifdef PFN_DOUBLE_PRECISION
		Vector3d position = Vector3d(0.0);
#else
		Vector3f position = Vector3f(0.0f);
#endif

		maths::Quat orientationQuat = glm::angleAxis(0.0f, glm::vec3{ 0.0f, 0.0f, -1.0f }); // Orientation of transform, expressed as quaternion in radians
		maths::EulerAngles orientationEulerAngles = { 0.0, 0.0, 0.0 }; // Orientation of transform, expressed as euler angles in degrees

		Vector3f scale = Vector3f(1.0f);

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(TransformComponent3D, position, orientationQuat, orientationEulerAngles, scale)
	};

	template<>
	inline void RegisterType<TransformComponent3D>()
	{
		entt::meta<TransformComponent3D>()
			.type(entt::hs("TransformComponent3D"))
			.data<&TransformComponent3D::position>(entt::hs("position"))
			.data<&TransformComponent3D::orientationQuat>(entt::hs("orientationQuat"))
			.data<&TransformComponent3D::orientationEulerAngles>(entt::hs("orientationEulerAngles"))
			.data<&TransformComponent3D::scale>(entt::hs("scale"));
	}

	// Update transform orientation with a new euler angles and recalculates quaternion
	inline void UpdateTransformOrientation(TransformComponent3D& transform, const maths::EulerAngles& eulerAnglesNew)
	{
		transform.orientationEulerAngles = eulerAnglesNew;
		transform.orientationQuat = maths::EulerToQuat({ -transform.orientationEulerAngles }) * glm::angleAxis(0.0f, glm::vec3{ 0.0f, 0.0f, -1.0f });
	}
}
