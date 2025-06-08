#include "resource/resource.h"

#include "resource/resource_data.h"
#include "resource/resource_loader.h"

namespace puffin
{
	Resource::Resource(fs::path path)
		: mPath(std::move(path))
	{

	}

	Resource::~Resource()
	{
		mResourceLoader = nullptr;
		mResourceData = nullptr;
		mPath.clear();
	}

	bool Resource::IsLoaded() const
	{
		if (!mResourceData)
			return false;

		return mResourceData->IsLoaded();
	}

	ResourceData* Resource::GetResourceData() const
	{
		return mResourceData.get();
	}

	ResourceLoader* Resource::GetResourceLoader() const
	{
		return mResourceLoader.get();
	}
}
