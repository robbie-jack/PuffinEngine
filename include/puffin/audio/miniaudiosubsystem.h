#pragma once

#include <miniaudio/miniaudio.h>

#include "puffin/audio/audiosubsystem.h"
#include "puffin/types/storage/mappedvector.h"

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

		void PlaySoundEffect(UUID soundAssetID) override;

		bool CreateSoundInstance(UUID soundAssetID, UUID soundInstanceID) override;
		void DestroySoundInstance(UUID soundInstanceID) override;

		bool StartSoundInstance(UUID soundInstanceID, bool restart) override;
		bool StopSoundInstance(UUID soundInstanceID) override;

	private:

		ma_engine* mSoundEngine = nullptr;

		MappedVector<UUID, ma_sound> mSounds;
	};
}
