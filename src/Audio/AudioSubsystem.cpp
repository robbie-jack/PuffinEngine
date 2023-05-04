#include "Audio/AudioSubsystem.h"

#include "Assets/AssetRegistry.h"
#include "Assets/SoundAsset.h"
#include "Engine/Engine.hpp"

#include <memory>

using namespace irrklang;

namespace puffin::Audio
{
	void AudioSubsystem::SetupCallbacks()
	{
		m_engine->registerCallback(Core::ExecutionStage::init, [&]() { Init(); }, "AudioSubsystem: Init", 50);
		m_engine->registerCallback(Core::ExecutionStage::subsystemUpdate, [&]() { Update(); }, "AudioSubsystem: Update");
		m_engine->registerCallback(Core::ExecutionStage::cleanup, [&]() { Cleanup(); }, "AudioSubsystem: Cleanup", 150);
	}

	void AudioSubsystem::Init()
	{

	}

	void AudioSubsystem::Update()
	{
		if (m_soundEngine)
		{
			ProcessSoundEvents();
		}
	}

	void AudioSubsystem::Cleanup()
	{
		StopAllSounds();

		if (m_soundEngine)
		{
			m_soundEngine->drop();
			m_soundEngine = nullptr;
		}
	}

	void AudioSubsystem::PlaySoundEffect(UUID soundId, float volume, bool looping, bool restart)
	{
		SoundEvent soundEvent;
		soundEvent.type = SoundEventType::PLAY;
		soundEvent.id = soundId;
		soundEvent.volume = volume;
		soundEvent.looping = looping;
		soundEvent.restart = restart;

		m_soundEventBuffer.Push(soundEvent);
	}

	UUID AudioSubsystem::PlaySoundEffect(const std::string& soundPath, float volume, bool looping, bool restart)
	{
		UUID soundId = Assets::AssetRegistry::Get()->GetAsset<Assets::SoundAsset>(soundPath)->ID();

		PlaySoundEffect(soundId, volume, looping, restart);

		return soundId;
	}

	void AudioSubsystem::StopSoundEffect(UUID soundId)
	{
		SoundEvent soundEvent;
		soundEvent.type = SoundEventType::STOP;
		soundEvent.id = soundId;

		m_soundEventBuffer.Push(soundEvent);
	}

	void AudioSubsystem::PauseSoundEffect(UUID soundId)
	{
		SoundEvent soundEvent;
		soundEvent.type = SoundEventType::PAUSE;
		soundEvent.id = soundId;

		m_soundEventBuffer.Push(soundEvent);
	}

	void AudioSubsystem::PlayAllSounds(bool forcePlay)
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

	void AudioSubsystem::PauseAllSounds()
	{
		for (const auto pair : m_activeSounds)
		{
			m_activeSoundsWasPaused[pair.first] = pair.second->getIsPaused();

			pair.second->setIsPaused(true);
		}
	}

	void AudioSubsystem::StopAllSounds()
	{
		for (const auto pair : m_activeSounds)
		{
			pair.second->stop();
			pair.second->drop();
		}

		m_activeSounds.clear();
	}

	void AudioSubsystem::ProcessSoundEvents()
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

	void AudioSubsystem::PlaySound(SoundEvent soundEvent)
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
		
			ISound* sound = m_soundEngine->play2D(soundPath.c_str(), soundEvent.looping, false, true);
			if (sound)
			{
				m_activeSounds[soundEvent.id] = sound;
				ModifySound(soundEvent);
			}
		}
	}

	void AudioSubsystem::ModifySound(SoundEvent soundEvent)
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

	void AudioSubsystem::PauseSound(SoundEvent soundEvent)
	{
		if (m_activeSounds.count(soundEvent.id))
		{
			m_activeSounds[soundEvent.id]->setIsPaused(true);
		}
	}

	void AudioSubsystem::StopSound(SoundEvent soundEvent)
	{
		if (m_activeSounds.count(soundEvent.id))
		{
			m_activeSounds[soundEvent.id]->stop();
			m_activeSounds[soundEvent.id]->drop();
			m_activeSounds.erase(soundEvent.id);
		}
	}
}