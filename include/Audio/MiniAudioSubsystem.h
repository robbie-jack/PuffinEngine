#pragma once

#include "Audio/AudioSubsystem.h"

#include "miniaudio/miniaudio.h"

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

		void playSound(PuffinID soundAssetID) override;

		bool createSoundInstance(PuffinID soundAssetID, PuffinID soundInstanceID) override;
		void destroySoundInstance(PuffinID soundInstanceID) override;

		bool startSoundInstance(PuffinID soundInstanceID, bool restart) override;
		bool stopSoundInstance(PuffinID soundInstanceID) override;

	private:

		ma_engine* mSoundEngine = nullptr;

		PackedVector<ma_sound> mSounds;
	};
}
