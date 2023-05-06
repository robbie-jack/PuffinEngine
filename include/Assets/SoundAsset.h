#pragma once

#include "Asset.h"

namespace puffin::assets
{
	static const std::string gSoundType = "Sound";
	static constexpr uint32_t gSoundVersion = 1; // Latest version of Static Mesh Asset Format

	class SoundAsset : public Asset
	{
	public:

		SoundAsset() : Asset(fs::path()) {}

		SoundAsset(const fs::path& path) : Asset(path) {}

		SoundAsset(const PuffinId id, const fs::path& path) : Asset(id, path) {}

		const std::string& type() const override
		{
			return gSoundType;
		}

		const uint32_t& version() const override
		{
			return gSoundVersion;
		}

		void unload() override {}
	};

}