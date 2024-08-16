#pragma once

#include <miniaudio/miniaudio.h>

#include "puffin/audio/audiosubsystem.h"
#include "puffin/types/packedvector.h"

namespace puffin::audio
{
	class MiniAudioSubsystem : public AudioSubsystemProvider
	{
	public:

		MiniAudioSubsystem(const std::shared_ptr<core::Engine>& engine);
		~MiniAudioSubsystem() override;

		void Initialize(core::SubsystemManager* subsystemManager) override;
		void Deinitialize() override;

		void Update(double deltaTime) override;

	protected:

		void PlaySoundEffect(PuffinID soundAssetID) override;

		bool CreateSoundInstance(PuffinID soundAssetID, PuffinID soundInstanceID) override;
		void DestroySoundInstance(PuffinID soundInstanceID) override;

		bool StartSoundInstance(PuffinID soundInstanceID, bool restart) override;
		bool StopSoundInstance(PuffinID soundInstanceID) override;

	private:

		ma_engine* mSoundEngine = nullptr;

		PackedVector<PuffinID, ma_sound> mSounds;
	};
}
