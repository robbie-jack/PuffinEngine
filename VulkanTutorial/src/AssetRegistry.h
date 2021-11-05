#pragma once

#include "Types/UUID.h"

#include <cereal/types/string.hpp>

#include <string>
#include <memory>
#include <unordered_map>
#include <filesystem>

namespace fs = std::filesystem;

namespace Puffin
{
	class Asset
	{
	public:

		Asset(fs::path path) : path_(path) {};

		UUID ID()
		{
			return id_;
		}

		fs::path RelativePath()
		{
			return path_;
		}

		void RelativePath(fs::path path)
		{
			path_ = path;
		}

		virtual std::string Type() = 0;
		virtual void Save() = 0;
		virtual void Load() = 0;
		virtual void Unload() = 0;

		template<class Archive>
		void save(Archive& archive) const
		{
			archive(cereal::make_nvp("UUID", id_));
			archive(cereal::make_nvp("Path", path_.string()));
		}

		template<class Archive>
		void load(Archive& archive)
		{
			std::string path;
			archive(cereal::make_nvp("UUID", id_));
			archive(cereal::make_nvp("Path", path));

			path_ = path;
		}

	protected:

		UUID id_; // UUID of Asset
		fs::path path_; // Relative Asset Path

		bool isLoaded_; // Is Asset Currently Loaded

	};

	/*
	* Asset Registry tracks all registered assets for the current project
	*/
	class AssetRegistry
	{
		static AssetRegistry* instance;

		AssetRegistry(){};

	public:

		static AssetRegistry* Get()
		{
			if (!instance)
				instance = new AssetRegistry();

			return instance;
		}

		void ProjectRoot(fs::path contentRootPath);
		fs::path ProjectRoot();

		fs::path ContentRoot();

		// Get Asset from Registry
		std::shared_ptr<Asset> GetAsset(const UUID& uuid);

		// Get Typed Asset from Registry
		template<typename AssetType>
		std::shared_ptr<AssetType> GetAsset(const UUID& uuid);

		// Register new Asset to Registry
		UUID RegisterAsset(std::shared_ptr<Asset> asset);

	private:

		fs::path projectRootPath_;

		// Map of ID's to Asset, generated at runtime
		std::unordered_map<UUID, std::shared_ptr<Asset>> idToAssetMap_;

	};
}