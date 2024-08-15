#include "puffin/assets/material_asset.h"

#include "puffin/assets/asset_registry.h"

namespace puffin::assets
{
	////////////////////////////////
	// Material Asset
	////////////////////////////////

	bool MaterialAsset::save()
	{
		const fs::path fullPath = AssetRegistry::get()->content_root() / relativePath();

		// Create AssetData Struct
		AssetData data;
		data.id = id();
		data.type = AssetType::Material;
		data.version = gMaterialAssetVersion;

		// Fill Metadata from Info struct
		data.json_data["vertex_shader"] = mVertexShaderID;
		data.json_data["fragment_shader"] = mFragmentShaderID;

		data.binaryBlob.resize(0);

		return saveJsonFile(fullPath, data);
	}

	bool MaterialAsset::load(bool loadHeaderOnly)
	{
		// Check if file is already loaded
		if (mIsLoaded)
			return true;

		// Check if file exists
		const fs::path fullPath = AssetRegistry::get()->content_root() / relativePath();
		if (!fs::exists(fullPath))
			return false;

		// Load Binary/Metadata
		AssetData data;
		if (!loadJsonFile(fullPath, data))
		{
			return false;
		}

		mVertexShaderID = data.json_data["vertex_shader"];
		mFragmentShaderID = data.json_data["fragment_shader"];

		mIsLoaded = true;
		return true;
	}

	////////////////////////////////
	// Material Instance Asset
	////////////////////////////////

	bool MaterialInstanceAsset::save()
	{
		const fs::path fullPath = AssetRegistry::get()->content_root() / relativePath();

		// Create AssetData Struct
		AssetData data;
		data.type = AssetType::MaterialInstance;
		data.version = gMaterialInstAssetVersion;
		data.id = id();

		// Fill Metadata from Info struct
		data.json_data["texture_ids"] = mTexIDs;
		data.json_data["material_data"] = mData;
		data.json_data["base_material"] = mBaseMaterial;

		data.binaryBlob.resize(0);

		return saveJsonFile(fullPath, data);
	}

	bool MaterialInstanceAsset::load(bool loadHeaderOnly)
	{
		// Check if file is already loaded
		if (mIsLoaded)
			return true;

		// Check if file exists
		const fs::path fullPath = AssetRegistry::get()->content_root() / relativePath();
		if (!fs::exists(fullPath))
			return false;

		// Load Binary/Metadata
		AssetData data;
		if (!loadJsonFile(fullPath, data))
		{
			return false;
		}

		mTexIDs = data.json_data["texture_ids"];
		mData = data.json_data["material_data"];
		mBaseMaterial = data.json_data["base_material"];

		mIsLoaded = true;
		return true;
	}
}
