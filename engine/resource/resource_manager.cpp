#include "resource/resource_manager.h"

namespace puffin
{
	ResourceManager::ResourceManager()
	{
		
	}

	ResourceManager::~ResourceManager()
	{
		
	}

	void ResourceManager::Initialize(const io::ProjectFile& projectFile, const fs::path& projectPath)
	{
		mProjectName = projectFile.name;
		mProjectPath = projectPath;
		mProjectPath.remove_filename();

		mEnginePath = FindEngineRoot(fs::current_path());
	}

	Resource* ResourceManager::GetResource(fs::path resourcePath)
	{
		// PUFFIN_TODO - Implement
		return nullptr;
	}

	Resource* ResourceManager::LoadResource(fs::path resourcePath)
	{
		// PUFFIN_TODO - Implement
		return nullptr;
	}

	void ResourceManager::LoadResourceAsync(fs::path resourcePath)
	{
		// PUFFIN_TODO - Implement
	}

	void ResourceManager::UnloadResource(fs::path resourcePath)
	{
		// PUFFIN_TODO - Implement
	}

	void ResourceManager::UnloadResourceAsync(fs::path resourcePath)
	{
		// PUFFIN_TODO - Implement
	}

	fs::path ResourceManager::GetProjectPath() const
	{
		return mProjectPath;
	}

	fs::path ResourceManager::GetEnginePath() const
	{
		return mEnginePath;
	}

	fs::path ResourceManager::FindEngineRoot(const fs::path& currentPath)
	{
		bool cmakeListsInDir = false;
		fs::path localPath = currentPath;

		while (!cmakeListsInDir)
		{
			for (const auto& entry : fs::recursive_directory_iterator(localPath))
			{
				if (const auto& entryPath = entry.path(); strcmp(entryPath.filename().string().c_str(), "PuffinEngine.txt") == 0)
				{
					cmakeListsInDir = true;
					localPath = entryPath.parent_path();
					break;
				}
			}

			if (!cmakeListsInDir)
			{
				localPath = localPath.parent_path();
			}
		}

		return localPath;
	}
}
