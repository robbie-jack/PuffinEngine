#pragma once

#include "Asset.h"

namespace puffin::Assets
{
	static const std::string G_SOUND_TYPE = "Sound";
	static constexpr uint32_t G_SOUND_VERSION = 1; // Latest version of Static Mesh Asset Format

	class SoundAsset : public Asset
	{
	public:

		SoundAsset() : Asset(fs::path()) {}

		SoundAsset(const fs::path& path) : Asset(path) {}

		SoundAsset(const UUID id, const fs::path& path) : Asset(id, path) {}

		const std::string& Type() const
		{
			return G_SOUND_TYPE;
		}

		const uint32_t& Version() const
		{
			return G_SOUND_VERSION;
		}

		void Unload() override {}
	};

}