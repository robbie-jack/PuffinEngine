#pragma once

#include "Types/Vector.h"
#include "Types/Quat.h"

#include "nlohmann/json.hpp"

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

		TransformComponent3D(const Vector3d& position_, const maths::Quat& orientation_, const Vector3f& scale_) :
			position(position_), orientation(orientation_), scale(scale_) {}
#else
		TransformComponent3D(const Vector3f& position_) : position(position_) {}

		TransformComponent3D(const Vector3f& position_, const maths::Quat& orientation_, const Vector3f& scale_) :
			position(position_), orientation(orientation_), scale(scale_) {}
#endif

		TransformComponent3D(const TransformComponent3D& t) : position(t.position), orientation(t.orientation), scale(t.scale) {}

		~TransformComponent3D() = default;

		TransformComponent3D& operator=(const TransformComponent3D& rhs) = default;

#ifdef PFN_DOUBLE_PRECISION
		Vector3d position = Vector3d(0.0);
#else
		Vector3f position = Vector3f(0.0f);
#endif

		maths::Quat orientation = angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 1.0));

		Vector3f scale = Vector3f(1.0f);

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(TransformComponent3D, position, orientation, scale)
	};
}
