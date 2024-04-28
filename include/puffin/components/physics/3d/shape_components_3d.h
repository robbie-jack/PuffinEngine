#pragma once

#include "puffin/types/Vector.h"

#include "nlohmann/json.hpp"

namespace puffin::physics
{
	struct ShapeComponent3D
	{
		Vector3f centreOfMass = Vector3f(0.0f);
	};

	struct SphereComponent3D : ShapeComponent3D
	{
		SphereComponent3D() = default;

		SphereComponent3D(const float& radius_) : radius(radius_) {}

		float radius = 0.5f;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(SphereComponent3D, centreOfMass, radius)
	};

	struct BoxComponent3D : ShapeComponent3D
	{
		BoxComponent3D() = default;

		BoxComponent3D(const Vector3f& halfExtent_) : halfExtent(halfExtent_) {}

		Vector3f halfExtent = { 0.5f };

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(BoxComponent3D, centreOfMass, halfExtent)
	};
}