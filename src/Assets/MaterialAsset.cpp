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
		data.id = id();

		// Fill Metadata from Info struct
		data.json["vertex_shader"] = mVertexShaderID;
		data.json["fragment_shader"] = mFragmentShaderID;

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

		mIsLoaded = true;
		return true;
	}

	bool MaterialInstanceAsset::save()
	{
		const fs::path fullPath = AssetRegistry::get()->contentRoot() / relativePath();

		// Create AssetData Struct
		AssetData data;
		data.type = AssetType::Material;
		data.version = gMaterialAssetVersion;
		data.id = id();

		// Fill Metadata from Info struct
		data.json["texture_ids"] = mTexIDs;
		data.json["material_data"] = mData;
		data.json["base_material"] = mBaseMaterial;

		data.binaryBlob.resize(0);

		return saveJsonFile(fullPath, data);
	}

	bool MaterialInstanceAsset::load()
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

		mTexIDs = data.json["texture_ids"];
		mData = data.json["material_data"];
		mBaseMaterial = data.json["base_material"];

		mIsLoaded = true;
		return true;
	}
}
