#pragma once

#include <Types/UUID.h>

#include "nlohmann/json.hpp"

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
}