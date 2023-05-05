#pragma once

#include "Types/Vector.h"
#include "Types/Quat.h"

#include "nlohmann/json.hpp"

namespace puffin
{
#ifdef PFN_USE_DOUBLE_PRECISION
	constexpr bool gUseDoublePrecision = true;
#else
	constexpr bool gUseDoublePrecision = false;
#endif

	struct TransformComponent
	{
		TransformComponent() = default;

#ifdef PFN_USE_DOUBLE_PRECISION
		TransformComponent(Vector3d InPosition, Maths::Quat InRotation, Vector3f InScale) :
			position(InPosition), rotation(InRotation), scale(InScale) {}
#else
		TransformComponent(Vector3f InPosition, Maths::Quat InRotation, Vector3f InScale) :
			position(InPosition), rotation(InRotation), scale(InScale) {}
#endif

		~TransformComponent() = default;

		TransformComponent& operator=(const TransformComponent& rhs) = default;

#ifdef PFN_USE_DOUBLE_PRECISION
		Vector3d position = Vector3d(0.0);
#else
		Vector3f position = Vector3f(0.0f);
#endif

		Maths::Quat rotation = Maths::Quat(0.0f, 0.0f, -1.0f, 0.0);

		Vector3f scale = Vector3f(1.0f);

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(TransformComponent, position, rotation, scale)
	};
}
