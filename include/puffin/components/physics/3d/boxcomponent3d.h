#pragma once

#include "nlohmann/json.hpp"

#include "puffin/components/physics/3d/shapecomponent3d.h"
#include "puffin/types/vector.h"

namespace puffin::physics
{
	struct BoxComponent3D : ShapeComponent3D
	{
		BoxComponent3D() = default;

		explicit BoxComponent3D(const Vector3f& halfExtent) : halfExtent(halfExtent) {}

		Vector3f halfExtent = { 0.5f };

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(BoxComponent3D, centreOfMass, halfExtent)
	};
}