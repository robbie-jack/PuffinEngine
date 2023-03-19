#include "Assets/AssetRegistry.h"
#include "Assets/MeshAsset.h"

#include <fstream>

namespace Puffin::Assets
{
	AssetRegistry* AssetRegistry::s_instance = 0;

	void AssetRegistry::ProjectName(const std::string& projectName)
	{
		m_projectName = projectName;
	}

	std::string AssetRegistry::ProjectName()
	{
		return m_projectName;
	}

	void AssetRegistry::ProjectRoot(fs::path projectRootPath)
	{
		m_projectRootPath = projectRootPath;
	}

	fs::path AssetRegistry::ProjectRoot()
	{
		return m_projectRootPath;
	}

	fs::path AssetRegistry::ContentRoot() const
	{
		return m_projectRootPath / "content";
	}

	void AssetRegistry::SaveAssetCache() const
	{
		// Fill AssetCache Struct
		AssetCache assetCache;

		for (const auto& [fst, snd] : m_idToAssetMap)
		{
			assetCache.data[fst].path = snd->RelativePath().string();
			assetCache.data[fst].type = snd->Type();
		}

		json data = assetCache;

		// Initialize Output File Stream and Cereal Binary Archive
		const fs::path assetCachePath = m_projectRootPath / (m_projectName + ".passetcache");
		std::ofstream os(assetCachePath, std::ios::out);

		os << std::setw(4) << data << std::endl;

		os.close();
	}

	void AssetRegistry::LoadAssetCache()
	{
		// Initialize Input File Stream and Cereal Binary Archive
		const fs::path assetCachePath = m_projectRootPath / (m_projectName + ".passetcache");
		std::ifstream is(assetCachePath);

		json data;
		is >> data;

		// Load AssetCache from file
		AssetCache assetCache = data;

		// Create Assets from factories based on their stored type
		for (const auto& [fst, snd] : assetCache.data)
		{
			std::string path = snd.path;
			std::string type = snd.type;
			std::shared_ptr<Asset> asset;

			for (const auto& factory : m_assetFactories)
			{
				if (factory->Type() == type)
				{
					asset = factory->AddAsset(fst, path);
					break;
				}
			}

			m_idToAssetMap[asset->ID()] = asset;
			m_pathToIDMap[asset->RelativePath().string()] = asset->ID();
		}
	}

	// Get Asset from Registry
	std::shared_ptr<Asset> AssetRegistry::GetAsset(const UUID& uuid)
	{
		// Return asset if it has been registered
		if (m_idToAssetMap.count(uuid))
		{
			return m_idToAssetMap[uuid];
		}

		// No asset with that ID has been registered, return nullptr
		return nullptr;
	}

	std::shared_ptr<Asset> AssetRegistry::GetAsset(const fs::path& path)
	{
		// Return asset if it has been registered
		if (m_pathToIDMap.count(path.string()))
		{
			return GetAsset(m_pathToIDMap[path.string()]);
		}
		
		// No asset with that path has been registered, return nullptr
		return nullptr;
	}
}