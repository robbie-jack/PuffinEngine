#pragma once

#include <Types/Vector.h>

#include "nlohmann/json.hpp"

//#define PFN_USE_DOUBLE_PRECISION

namespace Puffin
{
#ifdef PFN_USE_DOUBLE_PRECISION
	constexpr bool G_USE_DOUBLE_PRECISION = true;
#else
	constexpr bool G_USE_DOUBLE_PRECISION = false;
#endif

	struct TransformComponent
	{
		TransformComponent() = default;

#ifdef PFN_USE_DOUBLE_PRECISION
		TransformComponent(Vector3d InPosition, Vector3f InRotation, Vector3f InScale) :
			position(InPosition), rotation(InRotation), scale(InScale) {}
#else
		TransformComponent(Vector3f InPosition, Vector3f InRotation, Vector3f InScale) :
			position(InPosition), rotation(InRotation), scale(InScale) {}
#endif

		~TransformComponent() = default;

		TransformComponent& operator=(const TransformComponent& rhs) = default;

#ifdef PFN_USE_DOUBLE_PRECISION
		Vector3d position = Vector3d(0.0);
#else
		Vector3f position = Vector3f(0.0f);
#endif
		Vector3f rotation = Vector3f(0.0f);
		Vector3f scale = Vector3f(1.0f);

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(TransformComponent, position, rotation, scale)
	};
}
