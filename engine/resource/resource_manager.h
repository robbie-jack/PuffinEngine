#pragma once

#include <unordered_set>

#include "project_settings.h"

namespace puffin
{
	class Resource;

	/*
	 * Manages the lifetime of any resources created by it and ensures
	 * data is loaded & unloaded as needed
	 */
	class ResourceManager
	{
	public:

		explicit ResourceManager();
		~ResourceManager();

		void Initialize(const io::ProjectFile& projectFile, const fs::path& projectPath);

		Resource* GetResource(fs::path resourcePath);

		Resource* LoadResource(fs::path resourcePath);
		void LoadResourceAsync(fs::path resourcePath);

		void UnloadResource(fs::path resourcePath);
		void UnloadResourceAsync(fs::path resourcePath);

		fs::path GetProjectPath() const;
		fs::path GetEnginePath() const;

	private:

		static fs::path FindEngineRoot(const fs::path& currentPath);

		std::string mProjectName;
		fs::path mProjectPath;
		fs::path mEnginePath;

		std::unordered_map<fs::path, std::unique_ptr<Resource>> mResources;

	};
}
