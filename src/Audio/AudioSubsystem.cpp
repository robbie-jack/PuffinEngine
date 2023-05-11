#include "Audio/AudioSubsystem.h"

#include "Assets/AssetRegistry.h"
#include "Assets/SoundAsset.h"
#include "Core/Engine.h"

#include <memory>

using namespace irrklang;

namespace puffin::audio
{
	void AudioSubsystem::setupCallbacks()
	{
		mEngine->registerCallback(core::ExecutionStage::Init, [&]() { init(); }, "AudioSubsystem: Init", 50);
		mEngine->registerCallback(core::ExecutionStage::SubsystemUpdate, [&]() { update(); }, "AudioSubsystem: Update");
		mEngine->registerCallback(core::ExecutionStage::Shutdown, [&]() { shutdown(); }, "AudioSubsystem: Shutdown", 150);
	}

	void AudioSubsystem::init()
	{

	}

	void AudioSubsystem::update()
	{
		if (mSoundEngine)
		{
			processSoundEvents();
		}
	}

	void AudioSubsystem::shutdown()
	{
		stopAllSounds();

		if (mSoundEngine)
		{
			mSoundEngine->drop();
			mSoundEngine = nullptr;
		}
	}

	void AudioSubsystem::playSoundEffect(puffin::PuffinID soundId, float volume, bool looping, bool restart)
	{
		SoundEvent soundEvent;
		soundEvent.type = SoundEventType::Play;
		soundEvent.id = soundId;
		soundEvent.volume = volume;
		soundEvent.looping = looping;
		soundEvent.restart = restart;

		mSoundEventBuffer.push(soundEvent);
	}

	PuffinID AudioSubsystem::playSoundEffect(const std::string& soundPath, float volume, bool looping, bool restart)
	{
		const PuffinID soundId = assets::AssetRegistry::get()->getAsset<assets::SoundAsset>(soundPath)->id();

		playSoundEffect(soundId, volume, looping, restart);

		return soundId;
	}

	void AudioSubsystem::stopSoundEffect(PuffinID soundId)
	{
		SoundEvent soundEvent;
		soundEvent.type = SoundEventType::Stop;
		soundEvent.id = soundId;

		mSoundEventBuffer.push(soundEvent);
	}

	void AudioSubsystem::pauseSoundEffect(PuffinID soundId)
	{
		SoundEvent soundEvent;
		soundEvent.type = SoundEventType::Pause;
		soundEvent.id = soundId;

		mSoundEventBuffer.push(soundEvent);
	}

	void AudioSubsystem::playAllSounds(bool forcePlay)
	{
		for (const auto pair : mActiveSounds)
		{
			if (forcePlay)
			{
				pair.second->setIsPaused(false);
			}
			else
			{
				pair.second->setIsPaused(mActiveSoundsWasPaused[pair.first]);
			}
		}
	}

	void AudioSubsystem::pauseAllSounds()
	{
		for (const auto pair : mActiveSounds)
		{
			mActiveSoundsWasPaused[pair.first] = pair.second->getIsPaused();

			pair.second->setIsPaused(true);
		}
	}

	void AudioSubsystem::stopAllSounds()
	{
		for (const auto pair : mActiveSounds)
		{
			pair.second->stop();
			pair.second->drop();
		}

		mActiveSounds.clear();
	}

	void AudioSubsystem::processSoundEvents()
	{
		SoundEvent soundEvent;

		// Parse play sound buffer to play sounds
		while (!mSoundEventBuffer.isEmpty())
		{
			mSoundEventBuffer.pop(soundEvent);

			if (soundEvent.type == SoundEventType::Play)
			{
				playSound(soundEvent);
			}

			if (soundEvent.type == SoundEventType::Modify)
			{
				modifySound(soundEvent);
			}

			if (soundEvent.type == SoundEventType::Pause)
			{
				pauseSound(soundEvent);
			}

			if (soundEvent.type == SoundEventType::Stop)
			{
				stopSound(soundEvent);
			}
		}
	}

	void AudioSubsystem::playSound(SoundEvent soundEvent)
	{
		// Unpause sound if it already exists, and the restart flag has not been set
		if (mActiveSounds.count(soundEvent.id) && !soundEvent.restart)
		{
			mActiveSounds[soundEvent.id]->setIsPaused(false);
			return;
		}

		// If sound is not active, start playing it
		const auto soundAsset = std::static_pointer_cast<assets::SoundAsset>(assets::AssetRegistry::get()->getAsset(soundEvent.id));
		if (soundAsset)
		{
			const std::string& soundPath = (assets::AssetRegistry::get()->contentRoot() / soundAsset->relativePath()).string();
		
			ISound* sound = mSoundEngine->play2D(soundPath.c_str(), soundEvent.looping, false, true);
			if (sound)
			{
				mActiveSounds[soundEvent.id] = sound;
				modifySound(soundEvent);
			}
		}
	}

	void AudioSubsystem::modifySound(SoundEvent soundEvent)
	{
		if (mActiveSounds.count(soundEvent.id))
		{
			ISound* sound = mActiveSounds[soundEvent.id];
			if (sound)
			{
				sound->setVolume(soundEvent.volume);
			}
		}
	}

	void AudioSubsystem::pauseSound(SoundEvent soundEvent)
	{
		if (mActiveSounds.count(soundEvent.id))
		{
			mActiveSounds[soundEvent.id]->setIsPaused(true);
		}
	}

	void AudioSubsystem::stopSound(SoundEvent soundEvent)
	{
		if (mActiveSounds.count(soundEvent.id))
		{
			mActiveSounds[soundEvent.id]->stop();
			mActiveSounds[soundEvent.id]->drop();
			mActiveSounds.erase(soundEvent.id);
		}
	}
}