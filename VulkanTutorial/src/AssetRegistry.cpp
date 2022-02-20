#include "AssetRegistry.h"
#include "Assets/MeshAsset.h"

#include <fstream>

#include <cereal/archives/binary.hpp>

namespace Puffin
{
	AssetRegistry* AssetRegistry::s_instance = 0;

	void AssetRegistry::ProjectName(const std::string& projectName)
	{
		m_projectName = projectName;
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

		for (const auto& pair : m_idToAssetMap)
		{
			assetCache.paths[pair.first] = pair.second->RelativePath().string();
			assetCache.types[pair.first] = pair.second->Type();
		}

		// Initialize Output File Stream and Cereal Binary Archive
		const fs::path assetCachePath = m_projectRootPath / (m_projectName + ".passetcache");
		std::ofstream os(assetCachePath, std::ios::binary);
		cereal::BinaryOutputArchive archive(os);

		// Save AssetCache
		archive(assetCache);
	}

	void AssetRegistry::LoadAssetCache()
	{
		// Initialize Input File Stream and Cereal Binary Archive
		const fs::path assetCachePath = m_projectRootPath / (m_projectName + ".passetcache");
		std::ifstream is(assetCachePath, std::ios::binary);
		cereal::BinaryInputArchive archive(is);

		// Load AssetCache from file
		AssetCache assetCache;
		archive(assetCache);

		// Create Assets from factories based on their stored type
		for (const auto& pair : assetCache.paths)
		{
			std::string type = assetCache.types[pair.first];
			std::shared_ptr<Asset> asset;

			for (const auto& factory : m_assetFactories)
			{
				if (factory->Type() == type)
				{
					asset = factory->AddAsset(pair.first, pair.second);
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