#include "Assets/MaterialAsset.h"

#include "Assets/AssetRegistry.h"

namespace puffin::assets
{
	////////////////////////////////
	// Material Asset Base
	////////////////////////////////

	void MaterialAssetBase::dumpMaterialDataToJson(json& metadata) const
	{
		metadata["textures_ids"] = mTexIDs;
		metadata["material_data"] = mData;
	}

	void MaterialAssetBase::loadMaterialDataFromJson(const json& metadata)
	{
		mTexIDs = metadata["texture_ids"];
		mData = metadata["material_data"];
	}

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

		dumpMaterialDataToJson(metadata);

		metadata["vertex_shader"] = mVertexShaderID;
		metadata["fragment_shader"] = mVertexShaderID;

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

		loadMaterialDataFromJson(metadata);

		mVertexShaderID = metadata["vertex_shader"];
		mVertexShaderID = metadata["fragment_shader"];

		mIsLoaded = true;
		return true;
	}

	////////////////////////////////
	// Material Instance Asset
	////////////////////////////////

	bool MaterialInstanceAsset::save()
	{
		const fs::path fullPath = AssetRegistry::get()->contentRoot() / relativePath();

		// Create AssetData Struct
		AssetData data;
		data.type = gMaterialAssetType;
		data.version = gMaterialAssetVersion;

		// Fill Metadata from Info struct
		json metadata;

		dumpMaterialDataToJson(metadata);

		metadata["base_material"] = mBaseMaterial;
		metadata["texture_id_override"] = mTexIDsOverride;
		metadata["data_override"] = mDataOverride;

		data.json = metadata.dump();

		data.binaryBlob.resize(0);

		return saveBinaryFile(fullPath, data);
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
		if (!loadBinaryFile(fullPath, data))
		{
			return false;
		}

		const json metadata = json::parse(data.json);

		loadMaterialDataFromJson(metadata);

		mBaseMaterial = metadata["base_material"];
		mTexIDsOverride = metadata["texture_id_override"];
		mDataOverride = metadata["data_override"];

		mIsLoaded = true;
		return true;
	}
}
