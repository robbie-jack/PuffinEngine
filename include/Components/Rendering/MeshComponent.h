#pragma once

#include <Types/UUID.h>
#include "Types/Vector.h"
#include "Types/Vertex.hpp"

#include "nlohmann/json.hpp"

#include <vector>

namespace Puffin::Rendering
{
	struct MeshComponent
	{
		MeshComponent() {}
		
		MeshComponent(UUID InMeshID, UUID InTextureID) :
			meshAssetID(InMeshID), textureAssetID(InTextureID)
		{
		}

		// Mesh Data
		UUID meshAssetID;

		// Texture
		UUID textureAssetID;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(MeshComponent, meshAssetID, textureAssetID)
	};

	struct ProceduralMeshComponent
	{
		ProceduralMeshComponent() {}

		std::vector<Rendering::VertexPNTV32> vertices;
		std::vector<uint32_t> indices;

		ProceduralMeshComponent(UUID InTextureID) : textureAssetID(InTextureID) {}

		UUID textureAssetID;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProceduralMeshComponent, textureAssetID)
	};
}