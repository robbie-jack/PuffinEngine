#pragma once

#include "nlohmann/json.hpp"

#include "Types/Vector.h"

#include <cstdint>

namespace Puffin::Procedural
{
	struct PlaneComponent
	{
		Vector2f halfSize = { 10.f }; // Half size of plane
		Vector2i numQuads = { 10 }; // Number of quads that make up planes surface

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(PlaneComponent, halfSize, numQuads)
	};

	struct SphereComponent
	{
		double radius = 0.5f;
	};

	struct UVSphereComponent : public SphereComponent
	{
		Vector2i numSegments = { 10 };

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(UVSphereComponent, radius, numSegments)
	};

	struct IcoSphereComponent : public SphereComponent
	{
		int subdivisions = 10;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(IcoSphereComponent, radius, subdivisions)
	};

	struct TerrainComponent : public PlaneComponent
	{
		int64_t seed = 983758376;
		double heightMultiplier = 10.0;
		double frequency = 10.0;
		int octaves = 4;
		double frequencyMult = 2.0;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(TerrainComponent, halfSize, numQuads)
	};
}