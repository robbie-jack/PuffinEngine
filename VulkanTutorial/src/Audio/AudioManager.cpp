#include "AudioManager.h"

#include "Assets/AssetRegistry.h"
#include "Assets/SoundAsset.h"

#include <memory>

using namespace irrklang;

namespace Puffin::Audio
{
	void AudioManager::Init()
	{
		m_engine = createIrrKlangDevice();
	}

	void AudioManager::Update()
	{
		if (m_engine)
		{
			ProcessSoundEvents();
		}
	}

	void AudioManager::Cleanup()
	{
		StopAllSounds();

		if (m_engine)
		{
			m_engine->drop();
			m_engine = 0;
		}
	}

	void AudioManager::PlaySound(UUID soundId, float volume, bool looping, bool restart)
	{
		SoundEvent soundEvent;
		soundEvent.type = SoundEventType::PLAY;
		soundEvent.id = soundId;
		soundEvent.volume = volume;
		soundEvent.looping = looping;
		soundEvent.restart = restart;

		m_soundEventBuffer.Push(soundEvent);
	}

	void AudioManager::StopSound(UUID soundId)
	{
		SoundEvent soundEvent;
		soundEvent.type = SoundEventType::STOP;
		soundEvent.id = soundId;

		m_soundEventBuffer.Push(soundEvent);
	}

	void AudioManager::PauseSound(UUID soundId)
	{
		SoundEvent soundEvent;
		soundEvent.type = SoundEventType::PAUSE;
		soundEvent.id = soundId;

		m_soundEventBuffer.Push(soundEvent);
	}

	void AudioManager::PlayAllSounds(bool forcePlay)
	{
		for (const auto pair : m_activeSounds)
		{
			if (forcePlay)
			{
				pair.second->setIsPaused(false);
			}
			else
			{
				pair.second->setIsPaused(m_activeSoundsWasPaused[pair.first]);
			}
		}
	}

	void AudioManager::PauseAllSounds()
	{
		for (const auto pair : m_activeSounds)
		{
			m_activeSoundsWasPaused[pair.first] = pair.second->getIsPaused();

			pair.second->setIsPaused(true);
		}
	}

	void AudioManager::StopAllSounds()
	{
		for (const auto pair : m_activeSounds)
		{
			pair.second->stop();
			pair.second->drop();
		}

		m_activeSounds.clear();
	}

	void AudioManager::ProcessSoundEvents()
	{
		SoundEvent soundEvent;

		// Parse play sound buffer to play sounds
		while (!m_soundEventBuffer.IsEmpty())
		{
			m_soundEventBuffer.Pop(soundEvent);

			if (soundEvent.type == SoundEventType::PLAY)
			{
				PlaySound(soundEvent);
			}

			if (soundEvent.type == SoundEventType::MODIFY)
			{
				ModifySound(soundEvent);
			}

			if (soundEvent.type == SoundEventType::PAUSE)
			{
				PauseSound(soundEvent);
			}

			if (soundEvent.type == SoundEventType::STOP)
			{
				StopSound(soundEvent);
			}
		}
	}

	void AudioManager::PlaySound(SoundEvent soundEvent)
	{
		// Unpause sound if it already exists, and the restart flag has not been set
		if (m_activeSounds.count(soundEvent.id) && !soundEvent.restart)
		{
			m_activeSounds[soundEvent.id]->setIsPaused(false);
			return;
		}

		// If sound is not active, start playing it
		const auto soundAsset = std::static_pointer_cast<Assets::SoundAsset>(Assets::AssetRegistry::Get()->GetAsset(soundEvent.id));
		if (soundAsset)
		{
			const std::string& soundPath = (Assets::AssetRegistry::Get()->ContentRoot() / soundAsset->RelativePath()).string();
		
			ISound* sound = m_engine->play2D(soundPath.c_str(), soundEvent.looping, false, true);
			if (sound)
			{
				m_activeSounds[soundEvent.id] = sound;
				ModifySound(soundEvent);
			}
		}
	}

	void AudioManager::ModifySound(SoundEvent soundEvent)
	{
		if (m_activeSounds.count(soundEvent.id))
		{
			ISound* sound = m_activeSounds[soundEvent.id];
			if (sound)
			{
				sound->setVolume(soundEvent.volume);
			}
		}
	}

	void AudioManager::PauseSound(SoundEvent soundEvent)
	{
		if (m_activeSounds.count(soundEvent.id))
		{
			m_activeSounds[soundEvent.id]->setIsPaused(true);
		}
	}

	void AudioManager::StopSound(SoundEvent soundEvent)
	{
		if (m_activeSounds.count(soundEvent.id))
		{
			m_activeSounds[soundEvent.id]->stop();
			m_activeSounds[soundEvent.id]->drop();
			m_activeSounds.erase(soundEvent.id);
		}
	}
}