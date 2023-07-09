#include "Assets/MaterialAsset.h"

#include "Assets/AssetRegistry.h"

namespace puffin::assets
{
	////////////////////////////////
	// Material Asset
	////////////////////////////////

	bool MaterialAsset::save()
	{
		const fs::path fullPath = AssetRegistry::get()->contentRoot() / relativePath();

		// Create AssetData Struct
		AssetData data;
		data.type = gMaterialAssetType;
		data.version = gMaterialAssetVersion;

		// Fill Metadata from Info struct
		json metadata;

		metadata["vertex_shader"] = mVertexShaderID;
		metadata["fragment_shader"] = mVertexShaderID;
		metadata["textures_ids"] = mTexIDs;
		metadata["material_data"] = mData;
		metadata["base_material"] = mBaseMaterial;
		metadata["texture_id_override"] = mTexIDOverride;
		metadata["data_override"] = mDataOverride;

		data.json = metadata.dump();

		data.binaryBlob.resize(0);

		return saveBinaryFile(fullPath, data);
	}

	bool MaterialAsset::load()
	{
		// Check if file is already loaded
		if (mIsLoaded)
			return true;

		// Check if file exists
		const fs::path fullPath = AssetRegistry::get()->contentRoot() / relativePath();
		if (!fs::exists(fullPath))
			return false;

		// Load Binary/Metadata
		AssetData data;
		if (!loadBinaryFile(fullPath, data))
		{
			return false;
		}

		const json metadata = json::parse(data.json);

		mVertexShaderID = metadata["vertex_shader"];
		mVertexShaderID = metadata["fragment_shader"];
		mTexIDs = metadata["texture_ids"];
		mData = metadata["material_data"];
		mBaseMaterial = metadata["base_material"];
		mTexIDOverride = metadata["texture_id_override"];
		mDataOverride = metadata["data_override"];

		mIsLoaded = true;
		return true;
	}
}
