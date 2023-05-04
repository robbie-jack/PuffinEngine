#pragma once

#include "Asset.h"

#include <string>
#include <memory>
#include <unordered_map>
#include <filesystem>

#include "nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace puffin::Assets
{
	/*
	 * Asset Cache
	 * Struct which Asset ID/Path, ID/Type pairs are stored in when saving/loading
	 */
	struct AssetCacheData
	{
		std::string path;
		std::string type;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(AssetCacheData, path, type)
	};

	struct AssetCache
	{
		std::unordered_map<UUID, AssetCacheData> data;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(AssetCache, data)
	};

	/* Asset Factory Interface
	 * Virtual Interface for templated AssetFactories
	 */
	class IAssetFactory
	{
	public:
		virtual ~IAssetFactory() = default;
		virtual std::string Type() = 0;
		virtual std::shared_ptr<Asset> AddAsset(UUID id, fs::path path) = 0;
	};

	/*
	 * Asset Factory
	 * Template Class which is used for instantiating Assets at runtime
	 */
	template<typename AssetType>
	class AssetFactory : public IAssetFactory
	{
	public:

		AssetFactory()
		{
			AssetType* asset = new AssetType();
			m_assetTypeString = asset->Type();
			delete asset;
			asset = nullptr;
		}

		std::string Type()
		{
			return m_assetTypeString;
		}

		std::shared_ptr<Asset> AddAsset(UUID id, fs::path path) override
		{
			return std::make_shared<AssetType>(id, path);
		}

	private:

		std::string m_assetTypeString;

	};

	/*
	* Asset Registry tracks all registered assets for the current project
	*/
	class AssetRegistry
	{
		static AssetRegistry* s_instance;

		AssetRegistry() = default;

	public:

		static AssetRegistry* Get()
		{
			if (!s_instance)
				s_instance = new AssetRegistry();

			return s_instance;
		}

		static void Clear()
		{
			delete s_instance;
			s_instance = nullptr;
		}

		~AssetRegistry()
		{
			for (const auto& [fst, snd] : m_idToAssetMap)
			{
				snd->Unload();
			}

			m_idToAssetMap.clear();
			m_pathToIDMap.clear();
			m_assetFactories.clear();
		}

		void ProjectName(const std::string& projectName);
		std::string ProjectName();

		void ProjectRoot(fs::path contentRootPath);
		fs::path ProjectRoot();

		fs::path ContentRoot() const;

		// Asset Cache Saving/Loading
		void SaveAssetCache() const;
		void LoadAssetCache();

		// Get Asset from Registry
		std::shared_ptr<Asset> GetAsset(const UUID& uuid);

		std::shared_ptr<Asset> GetAsset(const fs::path& path);

		// Get Typed Asset from Registry
		template<typename AssetType>
		std::shared_ptr<AssetType> GetAsset(const UUID& uuid)
		{
			return std::static_pointer_cast<AssetType>(GetAsset(uuid));
		}

		template<typename AssetType>
		std::shared_ptr<AssetType> GetAsset(const fs::path& path)
		{
			return std::static_pointer_cast<AssetType>(GetAsset(path));
		}

		// Register new Asset to Registry
		template<typename AssetType>
		std::shared_ptr<AssetType> AddAsset(const fs::path& path)
		{
			// First check if there is already an asset using this path
			std::shared_ptr<AssetType> asset = GetAsset<AssetType>(path);
			if (asset == nullptr)
			{
				// If there isn't create a new asset
				asset = std::make_shared<AssetType>(path);

				m_idToAssetMap.insert({ asset->ID(), asset });
				m_pathToIDMap.insert({ asset->RelativePath().string(), asset->ID() });
			}

			// Return existing/created asset
			return asset;
		}

		template<typename AssetType>
		void RegisterAssetType()
		{
			std::shared_ptr<AssetFactory<AssetType>> factory = std::make_shared<AssetFactory<AssetType>>();

			m_assetFactories.push_back(factory);
		}

	private:

		std::string m_projectName;
		fs::path m_projectRootPath;

		// Map of ID's to Asset, generated at runtime
		std::unordered_map<UUID, std::shared_ptr<Asset>> m_idToAssetMap;
		std::unordered_map<std::string, UUID> m_pathToIDMap;

		// Asset Factories for creating assets at runtime
		std::vector<std::shared_ptr<IAssetFactory>> m_assetFactories;
	};
}