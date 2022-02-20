#pragma once

#include "Types/UUID.h"

#include <cereal/types/string.hpp>

#include <string>
#include <memory>
#include <unordered_map>
#include <set>
#include <filesystem>

#include <cereal/types/map.hpp>

#include "AssetRegistry.h"

namespace fs = std::filesystem;

namespace Puffin
{
	class Asset
	{
	public:

		Asset(fs::path path) : m_path(path) { m_isLoaded = false; }
		Asset(UUID uuid, fs::path path) : m_id(uuid), m_path(path) { m_isLoaded = false; }

		UUID ID()
		{
			return m_id;
		}

		fs::path RelativePath()
		{
			return m_path;
		}

		void RelativePath(fs::path path)
		{
			m_path = path;
		}

		virtual std::string Type() = 0;
		virtual bool Save() = 0;
		virtual bool Load() = 0;
		virtual void Unload() = 0;

		template<class Archive>
		void save(Archive& archive) const
		{
			archive(cereal::make_nvp("UUID", m_id));
			archive(cereal::make_nvp("Path", m_path.string()));
		}

		template<class Archive>
		void load(Archive& archive)
		{
			std::string path;
			archive(cereal::make_nvp("UUID", m_id));
			archive(cereal::make_nvp("Path", path));

			m_path = path;
		}

	private:

		UUID m_id; // UUID of Asset
		fs::path m_path; // Relative Asset Path

		bool m_isLoaded; // Is Asset Currently Loaded

	};

	/*
	 * Asset Cache
	 * Struct which Asset ID/Path, ID/Type pairs are stored in when saving/loading
	 */
	struct AssetCache
	{
		std::unordered_map<UUID, std::string> paths;
		std::unordered_map<UUID, std::string> types;

		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(paths);
			archive(types);
		}
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

		void ProjectName(const std::string& projectName);

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
			std::shared_ptr<AssetType> asset = std::make_shared<AssetType>(path);

			m_idToAssetMap.insert({ asset->ID(), asset });
			m_pathToIDMap.insert({ asset->RelativePath().string(), asset->ID() });
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
		std::unordered_map<std::string_view, UUID> m_pathToIDMap;

		// Asset Factories for creating assets at runtime
		std::vector<std::shared_ptr<IAssetFactory>> m_assetFactories;
	};
}