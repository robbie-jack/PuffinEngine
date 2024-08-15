#include "puffin/assets/materialasset.h"

#include "puffin/assets/assetregistry.h"

namespace puffin::assets
{
	////////////////////////////////
	// Material Asset
	////////////////////////////////

	MaterialAsset::MaterialAsset(): Asset(fs::path())
	{
		
	}

	MaterialAsset::MaterialAsset(const fs::path& path): Asset(path)
	{
		
	}

	MaterialAsset::MaterialAsset(const PuffinID id, const fs::path& path): Asset(id, path)
	{
		
	}

	const std::string& MaterialAsset::GetType() const
	{
		return gMaterialAssetType;
	}

	const uint32_t& MaterialAsset::GetVersion() const
	{
		return gMaterialAssetVersion;
	}

	bool MaterialAsset::Save()
	{
		const fs::path fullPath = AssetRegistry::Get()->GetContentRoot() / GetRelativePath();

		// Create AssetData Struct
		AssetData data;
		data.id = GetID();
		data.type = AssetType::Material;
		data.version = gMaterialAssetVersion;

		// Fill Metadata from Info struct
		data.jsonData["vertex_shader"] = mVertexShaderID;
		data.jsonData["fragment_shader"] = mFragmentShaderID;

		data.binaryBlob.resize(0);

		return SaveJsonFile(fullPath, data);
	}

	bool MaterialAsset::Load(bool loadHeaderOnly)
	{
		// Check if file is already loaded
		if (mIsLoaded)
			return true;

		// Check if file exists
		const fs::path fullPath = AssetRegistry::Get()->GetContentRoot() / GetRelativePath();
		if (!fs::exists(fullPath))
			return false;

		// Load Binary/Metadata
		AssetData data;
		if (!LoadJsonFile(fullPath, data))
		{
			return false;
		}

		mVertexShaderID = data.jsonData["vertex_shader"];
		mFragmentShaderID = data.jsonData["fragment_shader"];

		mIsLoaded = true;
		return true;
	}

	PuffinID MaterialAsset::GetVertexShaderID() const
	{
		return mVertexShaderID;
	}

	void MaterialAsset::SetVertexShaderID(const PuffinID vertID)
	{
		mVertexShaderID = vertID;
	}

	PuffinID MaterialAsset::GetFragmentShaderID() const
	{
		return mFragmentShaderID;
	}

	void MaterialAsset::SetFragmentShaderID(const PuffinID fragID)
	{
		mFragmentShaderID = fragID;
	}

	////////////////////////////////
	// Material Instance Asset
	////////////////////////////////

	MaterialInstanceAsset::MaterialInstanceAsset(): Asset(fs::path())
	{
	}

	MaterialInstanceAsset::MaterialInstanceAsset(const fs::path& path): Asset(path)
	{
	}

	MaterialInstanceAsset::MaterialInstanceAsset(const PuffinID id, const fs::path& path): Asset(id, path)
	{
	}

	const std::string& MaterialInstanceAsset::GetType() const
	{
		return gMaterialInstAssetType;
	}

	const uint32_t& MaterialInstanceAsset::GetVersion() const
	{
		return gMaterialInstAssetVersion;
	}

	bool MaterialInstanceAsset::Save()
	{
		const fs::path fullPath = AssetRegistry::Get()->GetContentRoot() / GetRelativePath();

		// Create AssetData Struct
		AssetData data;
		data.type = AssetType::MaterialInstance;
		data.version = gMaterialInstAssetVersion;
		data.id = GetID();

		// Fill Metadata from Info struct
		data.jsonData["texture_ids"] = mTexIDs;
		data.jsonData["material_data"] = mData;
		data.jsonData["base_material"] = mBaseMaterial;

		data.binaryBlob.resize(0);

		return SaveJsonFile(fullPath, data);
	}

	bool MaterialInstanceAsset::Load(bool loadHeaderOnly)
	{
		// Check if file is already loaded
		if (mIsLoaded)
			return true;

		// Check if file exists
		const fs::path fullPath = AssetRegistry::Get()->GetContentRoot() / GetRelativePath();
		if (!fs::exists(fullPath))
			return false;

		// Load Binary/Metadata
		AssetData data;
		if (!LoadJsonFile(fullPath, data))
		{
			return false;
		}

		mTexIDs = data.jsonData["texture_ids"];
		mData = data.jsonData["material_data"];
		mBaseMaterial = data.jsonData["base_material"];

		mIsLoaded = true;
		return true;
	}

	PuffinID MaterialInstanceAsset::GetBaseMaterialID() const
	{
		return mBaseMaterial;
	}

	void MaterialInstanceAsset::SetBaseMaterialID(const PuffinID matID)
	{
		mBaseMaterial = matID;
	}

	std::array<PuffinID, rendering::gNumTexturesPerMat>& MaterialInstanceAsset::GetTexIDs()
	{
		return mTexIDs;
	}

	std::array<float, rendering::gNumFloatsPerMat>& MaterialInstanceAsset::GetData()
	{
		return mData;
	}
}
