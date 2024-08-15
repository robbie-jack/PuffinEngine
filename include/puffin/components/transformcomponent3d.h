#pragma once

#include "puffin/types/vector.h"
#include "puffin/types/quat.h"

#include "nlohmann/json.hpp"
#include "puffin/types/eulerangles.h"

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
		TransformComponent3D(const Vector3d& position_) : position(position_) {}

		TransformComponent3D(const Vector3d& position_, const maths::Quat& orientation_, const maths::EulerAngles& euler_, const Vector3f& scale_) :
			position(position_), orientation(orientation_), orientation_euler_angles(euler_), scale(scale_) {}
#else
		TransformComponent3D(const Vector3f& position_) : position(position_) {}

		TransformComponent3D(const Vector3f& position_, const maths::Quat& orientation_, const maths::EulerAngles& euler_, const Vector3f& scale_) :
			position(position_), orientation_quat(orientation_), orientation_euler_angles(euler_), scale(scale_) {}
#endif

		TransformComponent3D(const TransformComponent3D& t) :
		position(t.position), orientation_quat(t.orientation_quat), orientation_euler_angles(t.orientation_euler_angles), scale(t.scale) {}

		~TransformComponent3D() = default;

		TransformComponent3D& operator=(const TransformComponent3D& rhs) = default;

#ifdef PFN_DOUBLE_PRECISION
		Vector3d position = Vector3d(0.0);
#else
		Vector3f position = Vector3f(0.0f);
#endif

		maths::Quat orientation_quat = glm::angleAxis(0.0f, glm::vec3{ 0.0f, 0.0f, -1.0f }); // Orientation of transform, expressed as quaternion in radians
		maths::EulerAngles orientation_euler_angles = { 0.0, 0.0, 0.0 }; // Orientation of transform, expressed as euler angles in degrees

		Vector3f scale = Vector3f(1.0f);

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(TransformComponent3D, position, orientation_quat, orientation_euler_angles, scale)
	};

	// Update transform orientation with a new euler angles and recalculates quaternion
	inline void update_transform_orientation(TransformComponent3D& transform, const maths::EulerAngles& euler_angles_new)
	{
		transform.orientation_euler_angles = euler_angles_new;
		transform.orientation_quat = maths::euler_to_quat({ -transform.orientation_euler_angles }) * glm::angleAxis(0.0f, glm::vec3{ 0.0f, 0.0f, -1.0f });
	}
}
