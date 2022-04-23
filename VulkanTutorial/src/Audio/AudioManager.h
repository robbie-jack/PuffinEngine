#pragma once

#include "irrKlang\irrKlang.h"

#include "Types/UUID.h"
#include "Types/RingBuffer.h"

#include <vector>
#include <unordered_map>

namespace Puffin::Audio
{
	enum class SoundEventType
	{
		NONE,
		PLAY,
		PAUSE,
		STOP,
		MODIFY
	};

	struct SoundEvent
	{
		SoundEventType type = SoundEventType::NONE;
		UUID id;
		float volume = 1.0f;
		bool looping = false;
		bool restart = false;
	};

	class AudioManager
	{
	public:

		void Init();

		void Update();

		void Cleanup();

		// Play Sound, If this sound is already active, but paused, start playing it again
		void PlaySoundEffect(UUID soundId, float volume = 1.0f, bool looping = false, bool restart = false);
		UUID PlaySoundEffect(const std::string& soundId, float volume = 1.0f, bool looping = false, bool restart = false);
		void StopSoundEffect(UUID soundId);
		void PauseSoundEffect(UUID soundId);

		void PlayAllSounds(bool forcePlay = false);
		void PauseAllSounds();
		void StopAllSounds();

	private:

		irrklang::ISoundEngine* m_engine;

		//std::vector<irrklang::ISoundSource*> soundSources; // Loaded Sound Sources
		std::unordered_map<UUID, irrklang::ISound*> m_activeSounds; // Active Sound Effects
		std::unordered_map<UUID, bool> m_activeSoundsWasPaused; // Store whether a sound was playing or not

		RingBuffer<SoundEvent> m_soundEventBuffer;

		void ProcessSoundEvents();

		void PlaySound(SoundEvent soundEvent);
		void ModifySound(SoundEvent soundEvent);
		void PauseSound(SoundEvent soundEvent);
		void StopSound(SoundEvent soundEvent);
	};
}