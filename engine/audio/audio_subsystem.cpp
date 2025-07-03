#include "audio/audio_subsystem.h"

#include <memory>

#include "core/engine.h"
#include "audio/mini_audio_subsystem.h"

namespace puffin::audio
{
	////////////////////////////////
	// AudioSubsystemProvider
	////////////////////////////////

	AudioSubsystemProvider::AudioSubsystemProvider(const std::shared_ptr<core::Engine>& engine)
		: EngineSubsystem(engine)
	{
	}

	AudioSubsystemProvider::~AudioSubsystemProvider()
	{
		m_engine = nullptr;
	}

	std::string_view AudioSubsystemProvider::GetName() const
	{
		return reflection::GetTypeString<AudioSubsystemProvider>();
	}

	////////////////////////////////
	// AudioSubsystem
	////////////////////////////////

	AudioSubsystem::AudioSubsystem(const std::shared_ptr<core::Engine>& engine) : EngineSubsystem(engine)
	{
	}

	AudioSubsystem::~AudioSubsystem()
	{
		m_engine = nullptr;
	}

	void AudioSubsystem::Initialize(core::SubsystemManager* subsystemManager)
	{
		EngineSubsystem::Initialize(subsystemManager);

		mAudioSubsystemProvider = new MiniAudioSubsystem(m_engine);
		mAudioSubsystemProvider->Initialize(subsystemManager);
	}

	void AudioSubsystem::Deinitialize()
	{
		EngineSubsystem::Deinitialize();

		mAudioSubsystemProvider->Deinitialize();

		delete mAudioSubsystemProvider;
		mAudioSubsystemProvider = nullptr;
	}

	void AudioSubsystem::Update(double deltaTime)
	{
		EngineSubsystem::Update(deltaTime);
	}

	std::string_view AudioSubsystem::GetName() const
	{
		return reflection::GetTypeString<AudioSubsystem>();
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

		mSoundInstances.Emplace(soundInstanceID, SoundInstance());

		mSoundInstances[soundInstanceID].instanceID = soundInstanceID;
		mSoundInstances[soundInstanceID].assetID = soundAssetID;

		mSoundInstanceIDs[soundAssetID].insert(soundInstanceID);

		return soundInstanceID;
	}

	void AudioSubsystem::DestroySoundInstance(UUID soundInstanceID)
	{
		const UUID soundAssetID = mSoundInstances[soundInstanceID].assetID;

		mSoundInstances.Erase(soundInstanceID);
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
