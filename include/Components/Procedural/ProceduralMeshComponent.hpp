#pragma once

#include "nlohmann/json.hpp"

#include "Types/Vector.h"

#include <cstdint>

namespace puffin::Procedural
{
	namespace Icosahedron
	{
		constexpr float X = .525731112119133606f;
		constexpr float Z = .850650808352039932f;
		constexpr float N = 0.f;

		static void VertexPositions(std::vector<Vector3f>& vertexPositions)
		{
			vertexPositions.clear();

			vertexPositions = 
			{
				{-X,N,Z}, {X,N,Z}, {-X,N,-Z}, {X,N,-Z},
				{N,Z,X}, {N,Z,-X}, {N,-Z,X}, {N,-Z,-X},
				{Z,X,N}, {-Z,X, N}, {Z,-X,N}, {-Z,-X, N}
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

	struct PlaneComponent
	{
		Vector2f halfSize = { 10.f }; // Half size of plane
		Vector2i numQuads = { 10 }; // Number of quads that make up planes surface

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(PlaneComponent, halfSize, numQuads)
	};

	struct CubeComponent
	{
		Vector3f halfSize = { 10.f }; // Half size of plane
		Vector3i numQuads = { 10 }; // Number of quads that make up planes surface

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(CubeComponent, halfSize, numQuads)
	};

	struct SphereComponent
	{
		double radius = 0.5;
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