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
		data.type = AssetType::Material;
		data.version = gMaterialAssetVersion;

		// Fill Metadata from Info struct
		data.json["vertex_shader"] = mVertexShaderID;
		data.json["fragment_shader"] = mFragmentShaderID;
		data.json["texture_ids"] = mTexIDs;
		data.json["material_data"] = mData;
		data.json["base_material"] = mBaseMaterial;
		data.json["texture_id_override"] = mTexIDOverride;
		data.json["data_override"] = mDataOverride;

		data.binaryBlob.resize(0);

		return saveJsonFile(fullPath, data);
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
		if (!loadJsonFile(fullPath, data))
		{
			return false;
		}

		mVertexShaderID = data.json["vertex_shader"];
		mFragmentShaderID = data.json["fragment_shader"];
		mTexIDs = data.json["texture_ids"];
		mData = data.json["material_data"];
		mBaseMaterial = data.json["base_material"];
		mTexIDOverride = data.json["texture_id_override"];
		mDataOverride = data.json["data_override"];

		mIsLoaded = true;
		return true;
	}
}
