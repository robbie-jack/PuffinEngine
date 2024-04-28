#pragma once

#include "audio_subsystem.h"

#include "miniaudio/miniaudio.h"

namespace puffin::audio
{
	class MiniAudioSubsystem : public AudioSubsystemProvider
	{
	public:

		MiniAudioSubsystem(const std::shared_ptr<core::Engine>& engine);
		~MiniAudioSubsystem() override { mEngine = nullptr; }

		void startup();
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
