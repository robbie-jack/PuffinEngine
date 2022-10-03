#pragma once

#include "nlohmann/json.hpp"

#include "Types/Vector.h"

namespace Puffin::Procedural
{
	struct ProceduralPlaneComponent
	{
		Vector2f size = { 1.0f }; // Size of Plane
		Vector2i numQuads = { 1 }; // Number of quads that make up planes surface

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProceduralPlaneComponent, size, numQuads)
	};
}