#pragma once

#include "nlohmann/json.hpp"

#include "puffin/types/vector.h"

#include <cstdint>

namespace puffin::procedural
{
	namespace icosahedron
	{
		constexpr float gX = .525731112119133606f;
		constexpr float gZ = .850650808352039932f;
		constexpr float gN = 0.f;

		static void VertexPositions(std::vector<Vector3f>& vertexPositions)
		{
			vertexPositions.clear();

			vertexPositions = 
			{
				{-gX,gN,gZ}, {gX,gN,gZ}, {-gX,gN,-gZ}, {gX,gN,-gZ},
				{gN,gZ,gX}, {gN,gZ,-gX}, {gN,-gZ,gX}, {gN,-gZ,-gX},
				{gZ,gX,gN}, {-gZ,gX, gN}, {gZ,-gX,gN}, {-gZ,-gX, gN}
			};
		}

		static void Indices(std::vector<uint32_t>& indices)
		{
			indices.clear();

			indices = 
			{
				0,4,1,0,9,4,9,5,4,4,5,8,4,8,1,
				8,10,1,8,3,10,5,3,8,5,2,3,2,7,3,
				7,10,3,7,6,10,7,11,6,11,0,6,0,1,6,
				6,1,10,9,0,11,9,11,2,9,2,5,7,2,11
			};
		}
	};

	struct ProceduralPlaneComponent
	{
		ProceduralPlaneComponent() = default;

		Vector2f halfSize = { 10.f }; // Half size of plane
		Vector2i quadCount = { 10 }; // Number of quads that make up planes surface along a single axis

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProceduralPlaneComponent, halfSize, quadCount)
	};

	struct ProceduralCubeComponent
	{
		ProceduralCubeComponent() = default;

		Vector3f halfSize = { 10.f }; // Half size of plane
		Vector3i quads = { 10 }; // Number of quads that make up planes surface

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProceduralCubeComponent, halfSize, quads)
	};

	struct SphereComponent
	{
		SphereComponent() = default;

		double radius = 0.5;
	};

	struct UvSphereComponent : public SphereComponent
	{
		UvSphereComponent() = default;

		Vector2i segments = { 10 };

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(UvSphereComponent, radius, segments)
	};

	struct IcoSphereComponent : public SphereComponent
	{
		IcoSphereComponent() = default;

		int subdivisions = 10;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(IcoSphereComponent, radius, subdivisions)
	};

	struct TerrainComponent : public ProceduralPlaneComponent
	{
		TerrainComponent() = default;

		int64_t seed = 983758376;
		double heightMult = 10.0;
		double frequency = 10.0;
		int octaves = 4;
		double frequencyMult = 2.0;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(TerrainComponent, halfSize, quadCount)
	};
}