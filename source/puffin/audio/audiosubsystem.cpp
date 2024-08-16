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

	void AudioSubsystem::PlaySound(PuffinID soundAssetID)
	{
		mAudioSubsystemProvider->PlaySoundEffect(soundAssetID);
	}

	PuffinID AudioSubsystem::CreateSoundInstance(PuffinID soundAssetID)
	{
		const PuffinID soundInstanceID = generate_id();

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

	void AudioSubsystem::DestroySoundInstance(PuffinID soundInstanceID)
	{
		const PuffinID soundAssetID = mSoundInstances[soundInstanceID].assetID;

		mSoundInstances.erase(soundInstanceID);
		mSoundInstanceIDs[soundAssetID].erase(soundInstanceID);
	}

	void AudioSubsystem::StartSoundInstance(PuffinID soundInstanceID, bool restart)
	{
		mAudioSubsystemProvider->StartSoundInstance(soundInstanceID, restart);
	}

	void AudioSubsystem::StopSoundInstance(PuffinID soundInstanceID)
	{
		mAudioSubsystemProvider->StopSoundInstance(soundInstanceID);
	}

	PuffinID AudioSubsystem::CreateAndStartSoundInstance(PuffinID soundAssetID)
	{
		const PuffinID soundInstanceID = CreateSoundInstance(soundAssetID);

		StartSoundInstance(soundInstanceID);

		return soundInstanceID;
	}

	const std::set<PuffinID>& AudioSubsystem::GetAllInstanceIDsForSound(PuffinID soundAssetID)
	{
		return mSoundInstanceIDs[soundAssetID];
	}

	SoundInstance& AudioSubsystem::GetSoundInstance(PuffinID soundInstanceID)
	{
		return mSoundInstances[soundInstanceID];
	}
}
