#pragma once

#include "puffin/assets/asset.h"

namespace puffin::assets
{
	static const std::string gSoundType = "Sound";
	static constexpr uint32_t gSoundVersion = 1; // Latest version of Static Mesh Asset Format

	class SoundAsset : public Asset
	{
	public:

		SoundAsset() : Asset(fs::path()) {}

		SoundAsset(const fs::path& path) : Asset(path) {}

		SoundAsset(const UUID id, const fs::path& path) : Asset(id, path) {}

		const std::string& GetType() const override
		{
			return gSoundType;
		}

		const uint32_t& GetVersion() const override
		{
			return gSoundVersion;
		}

		bool Save() override { return true; }

		bool Load(bool loadHeaderOnly = false) override { return true; }

		void Unload() override {}
	};

}