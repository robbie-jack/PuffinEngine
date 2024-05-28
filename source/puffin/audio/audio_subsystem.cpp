#include "puffin/audio/audio_subsystem.h"

#include <memory>

#include "puffin/core/engine.h"
#include "puffin/audio/mini_audio_subsystem.h"

namespace puffin::audio
{
	////////////////////////////////
	// AudioSubsystemProvider
	////////////////////////////////

	

	////////////////////////////////
	// AudioSubsystem
	////////////////////////////////

	AudioSubsystem::AudioSubsystem(const std::shared_ptr<core::Engine>& engine) : System(engine)
	{
		m_engine->register_callback(core::ExecutionStage::Startup, [&]() { startup(); }, "AudioSubsystem: Startup", 50);
		m_engine->register_callback(core::ExecutionStage::SubsystemUpdate, [&]() { update(); }, "AudioSubsystem: Update");
		m_engine->register_callback(core::ExecutionStage::Shutdown, [&]() { shutdown(); }, "AudioSubsystem: Shutdown", 150);

		mAudioSubsystemProvider = m_engine->register_system<MiniAudioSubsystem>();
	}

	void AudioSubsystem::startup()
	{

	}

	void AudioSubsystem::update()
	{
		
	}

	void AudioSubsystem::shutdown()
	{
		
	}

	void AudioSubsystem::playSound(PuffinID soundAssetID)
	{
		mAudioSubsystemProvider->playSound(soundAssetID);
	}

	PuffinID AudioSubsystem::createSoundInstance(PuffinID soundAssetID)
	{
		const PuffinID soundInstanceID = generateID();

		if (!mAudioSubsystemProvider->createSoundInstance(soundAssetID, soundInstanceID))
		{
			return gInvalidID;
		}

		mSoundInstances.emplace(soundInstanceID, SoundInstance());

		mSoundInstances[soundInstanceID].instanceID = soundInstanceID;
		mSoundInstances[soundInstanceID].assetID = soundAssetID;

		mSoundInstanceIDs[soundAssetID].insert(soundInstanceID);

		return soundInstanceID;
	}

	void AudioSubsystem::destroySoundInstance(PuffinID soundInstanceID)
	{
		const PuffinID soundAssetID = mSoundInstances[soundInstanceID].assetID;

		mSoundInstances.erase(soundInstanceID);
		mSoundInstanceIDs[soundAssetID].erase(soundInstanceID);
	}

	void AudioSubsystem::startSoundInstance(PuffinID soundInstanceID, bool restart)
	{
		mAudioSubsystemProvider->startSoundInstance(soundInstanceID, restart);
	}

	void AudioSubsystem::stopSoundInstance(PuffinID soundInstanceID)
	{
		mAudioSubsystemProvider->stopSoundInstance(soundInstanceID);
	}

	PuffinID AudioSubsystem::createAndStartSoundInstance(PuffinID soundAssetID)
	{
		const PuffinID soundInstanceID = createSoundInstance(soundAssetID);

		startSoundInstance(soundInstanceID);

		return soundInstanceID;
	}

	const std::set<PuffinID>& AudioSubsystem::getAllInstanceIDsForSound(PuffinID soundAssetID)
	{
		return mSoundInstanceIDs[soundAssetID];
	}

	SoundInstance& AudioSubsystem::getSoundInstance(PuffinID soundInstanceID)
	{
		return mSoundInstances[soundInstanceID];
	}
}
