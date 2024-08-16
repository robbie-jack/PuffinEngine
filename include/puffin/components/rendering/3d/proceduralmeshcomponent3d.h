#pragma once

#include <vector>

#include "puffin/types/uuid.h"
#include "puffin/types/vertex.h"

namespace puffin::rendering
{
	struct ProceduralMeshComponent3D
	{
		ProceduralMeshComponent3D() = default;

		std::vector<rendering::VertexPNTV32> vertices;
		std::vector<uint32_t> indices;

		explicit ProceduralMeshComponent3D(const UUID textureID) : materialID(textureID) {}

		UUID materialID;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProceduralMeshComponent3D, materialID)
	};
}
