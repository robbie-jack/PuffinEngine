#pragma once

#include <Types/UUID.h>
#include "Types/Vector.h"
#include "Types\Vertex.h"

#include "nlohmann/json.hpp"

#include <vector>

namespace puffin::rendering
{
	struct MeshComponent
	{
		MeshComponent() {}
		
		MeshComponent(PuffinId meshId, PuffinId textureId) :
			meshAssetId(meshId), textureAssetId(textureId)
		{
		}

		// Mesh Data
		PuffinId meshAssetId;

		// Texture
		PuffinId textureAssetId;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(MeshComponent, meshAssetId, textureAssetId)
	};

	struct ProceduralMeshComponent
	{
		ProceduralMeshComponent() {}

		std::vector<rendering::VertexPNTV32> vertices;
		std::vector<uint32_t> indices;

		explicit ProceduralMeshComponent(PuffinId textureId) : textureAssetId(textureId) {}

		PuffinId textureAssetId;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProceduralMeshComponent, textureAssetId)
	};
}