#pragma once

#include "irrKlang\irrKlang.h"

#include "Engine/Subsystem.hpp"

#include "Types/UUID.h"
#include "Types/RingBuffer.h"

#include <vector>
#include <unordered_map>

namespace puffin::audio
{
	enum class SoundEventType
	{
		None,
		Play,
		Pause,
		Stop,
		Modify
	};

	struct SoundEvent
	{
		SoundEventType type = SoundEventType::None;
		UUID id;
		float volume = 1.0f;
		bool looping = false;
		bool restart = false;
	};

	class AudioSubsystem : public core::Subsystem
	{
	public:

		AudioSubsystem() = default;
		~AudioSubsystem() override = default;

		void setupCallbacks() override;

		void init();
		void update();
		void cleanup();

		// Play Sound, If this sound is already active, but paused, start playing it again
		void playSoundEffect(UUID soundId, float volume = 1.0f, bool looping = false, bool restart = false);
		UUID playSoundEffect(const std::string& soundPath, float volume = 1.0f, bool looping = false, bool restart = false);
		void stopSoundEffect(UUID soundId);
		void pauseSoundEffect(UUID soundId);

		void playAllSounds(bool forcePlay = false);
		void pauseAllSounds();
		void stopAllSounds();

	private:

		irrklang::ISoundEngine* mSoundEngine = nullptr;

		//std::vector<irrklang::ISoundSource*> soundSources; // Loaded Sound Sources
		std::unordered_map<UUID, irrklang::ISound*> mActiveSounds; // Active Sound Effects
		std::unordered_map<UUID, bool> mActiveSoundsWasPaused; // Store whether a sound was playing or not

		RingBuffer<SoundEvent> mSoundEventBuffer;

		void processSoundEvents();

		void playSound(SoundEvent soundEvent);
		void modifySound(SoundEvent soundEvent);
		void pauseSound(SoundEvent soundEvent);
		void stopSound(SoundEvent soundEvent);
	};
}
