#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace puffin
{
	class ResourceData;

	/*
	 *	Handles loading resource data from a file on disk such as
	 *	png, jpg, obj, gltf or custom formats
	 */
	class ResourceLoader
	{
	public:

		virtual ~ResourceLoader() = 0;

		virtual bool Load(fs::path path, ResourceData* data) const = 0;
		virtual bool LoadAsync(fs::path path, ResourceData* data) const = 0;

	private:



	};
}