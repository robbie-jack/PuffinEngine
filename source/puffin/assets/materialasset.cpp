#include "puffin/assets/materialasset.h"

#include "puffin/assets/assetregistry.h"

namespace puffin::assets
{
	////////////////////////////////
	// Material Asset
	////////////////////////////////

	bool MaterialAsset::Save()
	{
		const fs::path fullPath = AssetRegistry::get()->content_root() / GetRelativePath();

		// Create AssetData Struct
		AssetData data;
		data.ID = GetID();
		data.type = AssetType::Material;
		data.version = gMaterialAssetVersion;

		// Fill Metadata from Info struct
		data.json_data["vertex_shader"] = mVertexShaderID;
		data.json_data["fragment_shader"] = mFragmentShaderID;

		data.binaryBlob.resize(0);

		return SaveJsonFile(fullPath, data);
	}

	bool MaterialAsset::Load(bool loadHeaderOnly)
	{
		// Check if file is already loaded
		if (mIsLoaded)
			return true;

		// Check if file exists
		const fs::path fullPath = AssetRegistry::get()->content_root() / GetRelativePath();
		if (!fs::exists(fullPath))
			return false;

		// Load Binary/Metadata
		AssetData data;
		if (!LoadJsonFile(fullPath, data))
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

	bool MaterialInstanceAsset::Save()
	{
		const fs::path fullPath = AssetRegistry::get()->content_root() / GetRelativePath();

		// Create AssetData Struct
		AssetData data;
		data.type = AssetType::MaterialInstance;
		data.version = gMaterialInstAssetVersion;
		data.ID = GetID();

		// Fill Metadata from Info struct
		data.json_data["texture_ids"] = mTexIDs;
		data.json_data["material_data"] = mData;
		data.json_data["base_material"] = mBaseMaterial;

		data.binaryBlob.resize(0);

		return SaveJsonFile(fullPath, data);
	}

	bool MaterialInstanceAsset::Load(bool loadHeaderOnly)
	{
		// Check if file is already loaded
		if (mIsLoaded)
			return true;

		// Check if file exists
		const fs::path fullPath = AssetRegistry::get()->content_root() / GetRelativePath();
		if (!fs::exists(fullPath))
			return false;

		// Load Binary/Metadata
		AssetData data;
		if (!LoadJsonFile(fullPath, data))
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
