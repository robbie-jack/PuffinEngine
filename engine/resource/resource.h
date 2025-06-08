#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace puffin
{
	class ResourceLoader;
	class ResourceData;

	/*
	 * Representation of an asset or file loaded from disk, such as a mesh or image
	 * cano be used on its own for inline loading of a resource, or in conjunction
	 * with a resource manager for managed lifetime
	 */
	class Resource
	{
	public:

		Resource(fs::path path);
		virtual ~Resource();

		virtual bool Load() = 0;
		virtual bool LoadAsync() = 0;
		virtual bool Unload() = 0;
		virtual bool UnloadAsync() = 0;

		bool IsLoaded() const;

	protected:

		template<typename T>
		T* GetResourceData() const
		{
			return dynamic_cast<T*>(mResourceData);
		}

		ResourceData* GetResourceData() const;

		template<typename T>
		T* GetResourceLoader() const
		{
			return dynamic_cast<T*>(mResourceLoader);
		}

		ResourceLoader* GetResourceLoader() const;

	private:

		fs::path mPath;
		std::unique_ptr<ResourceData> mResourceData = nullptr;
		std::unique_ptr<ResourceLoader> mResourceLoader = nullptr;

	};
}