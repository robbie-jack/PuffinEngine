#pragma once

#include "nlohmann/json.hpp"

#include "Types/Vector.h"
#include "Types/Vertex.hpp"

#include <vector>

namespace Puffin::Rendering::Procedural
{
	struct ProcecuralMeshComponent
	{
		std::vector<Rendering::Vertex_PNTV_32> vertices;
		std::vector<uint32_t> indices;
	};

	struct ProceduralPlaneComponent : public ProcecuralMeshComponent
	{
		Vector2f halfSize = { 10.f }; // Half size of plane
		Vector2i numQuads = { 10 }; // Number of quads that make up planes surface

		UUID textureAssetID;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProceduralPlaneComponent, halfSize, numQuads)
	};
}