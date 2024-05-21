#pragma once

#include "puffin/types/Vector.h"

#include "nlohmann/json.hpp"

namespace puffin::physics
{
	struct ShapeComponent3D
	{
		Vector3f centre_of_mass = Vector3f(0.0f);
	};

	struct SphereComponent3D : ShapeComponent3D
	{
		SphereComponent3D() = default;

		SphereComponent3D(const float& radius_) : radius(radius_) {}

		float radius = 0.5f;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(SphereComponent3D, centre_of_mass, radius)
	};

	struct BoxComponent3D : ShapeComponent3D
	{
		BoxComponent3D() = default;

		BoxComponent3D(const Vector3f& halfExtent_) : half_extent(halfExtent_) {}

		Vector3f half_extent = { 0.5f };

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(BoxComponent3D, centre_of_mass, half_extent)
	};
}