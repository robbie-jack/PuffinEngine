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

	PuffinID AudioSubsystemProvider::createSoundInstance(PuffinID soundAssetID)
	{
		const PuffinID soundInstanceID = generateID();

		mSoundInstances.emplace(soundInstanceID, SoundInstance());

		mSoundInstances[soundInstanceID].instanceID = soundInstanceID;
		mSoundInstances[soundInstanceID].assetID = soundAssetID;

		mSoundInstanceIDs[soundAssetID].insert(soundInstanceID);

		createSoundInstanceInternal(soundAssetID, soundInstanceID);

		return soundInstanceID;
	}

	void AudioSubsystemProvider::destroySoundInstance(PuffinID soundInstanceID)
	{
		const PuffinID soundAssetID = mSoundInstances[soundInstanceID].assetID;

		mSoundInstances.erase(soundInstanceID);
		mSoundInstanceIDs[soundAssetID].erase(soundInstanceID);
	}

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
}