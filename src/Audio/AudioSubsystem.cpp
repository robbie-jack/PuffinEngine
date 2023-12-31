#include "Audio/AudioSubsystem.h"

#include "Assets/AssetRegistry.h"
#include "Assets/SoundAsset.h"
#include "Core/Engine.h"
#include "Audio/MiniAudioSubsystem.h"

#include <memory>

using namespace irrklang;

namespace puffin::audio
{
	////////////////////////////////
	// AudioSubsystemProvider
	////////////////////////////////

	

	////////////////////////////////
	// AudioSubsystem
	////////////////////////////////

	void AudioSubsystem::setup()
	{
		mEngine->registerCallback(core::ExecutionStage::Init, [&]() { init(); }, "AudioSubsystem: Init", 50);
		mEngine->registerCallback(core::ExecutionStage::SubsystemUpdate, [&]() { update(); }, "AudioSubsystem: Update");
		mEngine->registerCallback(core::ExecutionStage::Shutdown, [&]() { shutdown(); }, "AudioSubsystem: Shutdown", 150);

		mAudioSubsystemProvider = mEngine->registerSubsystem<MiniAudioSubsystem>();
	}

	void AudioSubsystem::init()
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
	}

	void AudioSubsystem::stopSoundInstance(PuffinID soundInstanceID)
	{
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
