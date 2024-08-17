#include "puffin/audio/audiosubsystem.h"

#include <memory>

#include "puffin/core/engine.h"
#include "puffin/audio/miniaudiosubsystem.h"

namespace puffin::audio
{
	////////////////////////////////
	// AudioSubsystemProvider
	////////////////////////////////

	AudioSubsystemProvider::AudioSubsystemProvider(const std::shared_ptr<core::Engine>& engine): Subsystem(engine)
	{
	}

	AudioSubsystemProvider::~AudioSubsystemProvider()
	{
		mEngine = nullptr;
	}

	////////////////////////////////
	// AudioSubsystem
	////////////////////////////////

	AudioSubsystem::AudioSubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
		mName = "AudioSubsystem";
	}

	AudioSubsystem::~AudioSubsystem()
	{
		mEngine = nullptr;
	}

	void AudioSubsystem::Initialize(core::SubsystemManager* subsystemManager)
	{
		Subsystem::Initialize(subsystemManager);

		mAudioSubsystemProvider = new MiniAudioSubsystem(mEngine);
		mAudioSubsystemProvider->Initialize(subsystemManager);
	}

	void AudioSubsystem::Deinitialize()
	{
		Subsystem::Deinitialize();

		mAudioSubsystemProvider->Deinitialize();

		delete mAudioSubsystemProvider;
		mAudioSubsystemProvider = nullptr;
	}

	void AudioSubsystem::Update(double deltaTime)
	{
		
	}

	void AudioSubsystem::PlaySound(UUID soundAssetID)
	{
		mAudioSubsystemProvider->PlaySoundEffect(soundAssetID);
	}

	UUID AudioSubsystem::CreateSoundInstance(UUID soundAssetID)
	{
		const UUID soundInstanceID = GenerateId();

		if (!mAudioSubsystemProvider->CreateSoundInstance(soundAssetID, soundInstanceID))
		{
			return gInvalidID;
		}

		mSoundInstances.emplace(soundInstanceID, SoundInstance());

		mSoundInstances[soundInstanceID].instanceID = soundInstanceID;
		mSoundInstances[soundInstanceID].assetID = soundAssetID;

		mSoundInstanceIDs[soundAssetID].insert(soundInstanceID);

		return soundInstanceID;
	}

	void AudioSubsystem::DestroySoundInstance(UUID soundInstanceID)
	{
		const UUID soundAssetID = mSoundInstances[soundInstanceID].assetID;

		mSoundInstances.erase(soundInstanceID);
		mSoundInstanceIDs[soundAssetID].erase(soundInstanceID);
	}

	void AudioSubsystem::StartSoundInstance(UUID soundInstanceID, bool restart)
	{
		mAudioSubsystemProvider->StartSoundInstance(soundInstanceID, restart);
	}

	void AudioSubsystem::StopSoundInstance(UUID soundInstanceID)
	{
		mAudioSubsystemProvider->StopSoundInstance(soundInstanceID);
	}

	UUID AudioSubsystem::CreateAndStartSoundInstance(UUID soundAssetID)
	{
		const UUID soundInstanceID = CreateSoundInstance(soundAssetID);

		StartSoundInstance(soundInstanceID);

		return soundInstanceID;
	}

	const std::set<UUID>& AudioSubsystem::GetAllInstanceIDsForSound(UUID soundAssetID)
	{
		return mSoundInstanceIDs[soundAssetID];
	}

	SoundInstance& AudioSubsystem::GetSoundInstance(UUID soundInstanceID)
	{
		return mSoundInstances[soundInstanceID];
	}
}
