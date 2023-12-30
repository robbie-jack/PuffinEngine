#pragma once

#include "Audio/AudioSubsystem.h"

class ma_engine;

namespace puffin::audio
{
	class MiniAudioSubsystem : public AudioSubsystemProvider
	{
	public:

		MiniAudioSubsystem() = default;
		~MiniAudioSubsystem() override = default;

		void setup() override;

		void init();
		void update();
		void shutdown();

	protected:

		void createSoundInstanceInternal(PuffinID soundAssetID, PuffinID soundInstanceID) override;

		void destroySoundInstanceInternal(PuffinID soundInstanceID) override;

	private:

		ma_engine* mSoundEngine = nullptr;

	};
}
