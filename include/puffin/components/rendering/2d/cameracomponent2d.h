#pragma once

#include "nlohmann/json.hpp"

#include "puffin/types/vector2.h"
#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"

namespace puffin
{
	namespace rendering
	{
		struct CameraComponent2D
		{
			CameraComponent2D() = default;

			bool active = false;
			Vector2f offset;
			float rotation = 0.0f;
			float zoom = 0.0f;
		};
	}
}