#include "puffin/audio/miniaudiosubsystem.h"

#define MINIAUDIO_IMPLEMENTATION
#include "puffin/audio/miniaudiosubsystem.h"

#include "miniaudio/miniaudio.h"

#include <iostream>

#include "puffin/core/engine.h"
#include "puffin/assets/assetregistry.h"
#include "puffin/assets/soundasset.h"
#include "puffin/audio/audiosubsystem.h"

namespace puffin::audio
{
	MiniAudioSubsystem::MiniAudioSubsystem(const std::shared_ptr<core::Engine>& engine) : audio::AudioSubsystemProvider(engine)
	{
	}

	MiniAudioSubsystem::~MiniAudioSubsystem()
	{
		mEngine = nullptr;
	}

	void MiniAudioSubsystem::Initialize(core::SubsystemManager* subsystemManager)
	{
		AudioSubsystemProvider::Initialize(subsystemManager);

		mSoundEngine = new ma_engine();

		if (const ma_result result = ma_engine_init(nullptr, mSoundEngine); result != MA_SUCCESS)
		{
			std::cout << "Failed to Initialize MiniAudio Sound Engine" << std::endl;
		}
	}

	void MiniAudioSubsystem::Deinitialize()
	{
		AudioSubsystemProvider::Deinitialize();

		for (auto& sound : mSounds)
		{
			ma_sound_uninit(&sound);
		}

		mSounds.clear();

		if (mSoundEngine)
		{
			ma_engine_uninit(mSoundEngine);
		}
	}

	void MiniAudioSubsystem::Update(double deltaTime)
	{
		AudioSubsystemProvider::Update(deltaTime);
	}

	void MiniAudioSubsystem::PlaySoundEffect(UUID soundAssetID)
	{
		auto soundAsset = assets::AssetRegistry::Get()->GetAsset<assets::SoundAsset>(soundAssetID);
		auto soundPath = (assets::AssetRegistry::Get()->GetContentRoot() / soundAsset->GetRelativePath()).string();

		ma_engine_play_sound(mSoundEngine, soundPath.c_str(), nullptr);
	}

	bool MiniAudioSubsystem::CreateSoundInstance(UUID soundAssetID, UUID soundInstanceID)
	{
		if (mSounds.contains(soundInstanceID))
		{
			return true;
		}

		auto soundAsset = assets::AssetRegistry::Get()->GetAsset<assets::SoundAsset>(soundAssetID);
		auto soundPath = (assets::AssetRegistry::Get()->GetContentRoot() / soundAsset->GetRelativePath()).string();

		mSounds.emplace(soundInstanceID, ma_sound());

		ma_result result;
		result = ma_sound_init_from_file(mSoundEngine, soundPath.c_str(), 0, nullptr, nullptr, &mSounds[soundInstanceID]);
		if (result != MA_SUCCESS)
		{
			std::cout << "Failed to initialize sound instance for: " << soundPath << std::endl;
			return false;
		}

		return true;
	}

	void MiniAudioSubsystem::DestroySoundInstance(UUID soundInstanceID)
	{
		if (!mSounds.contains(soundInstanceID))
		{
			return;
		}

		ma_sound_uninit(&mSounds[soundInstanceID]);
		mSounds.erase(soundInstanceID);
	}

	bool MiniAudioSubsystem::StartSoundInstance(UUID soundInstanceID, bool restart)
	{
		if (mSounds.contains(soundInstanceID))
		{
			std::cout << "Sound instance was not initialized: " << soundInstanceID << std::endl;
			return false;
		}

		ma_result result;
		if (restart)
		{
			result = ma_sound_seek_to_pcm_frame(&mSounds[soundInstanceID], 0);
			if (result != MA_SUCCESS)
			{
				std::cout << "Failed to restart sound instance: " << soundInstanceID << std::endl;
				return false;
			}
		}
				
		result = ma_sound_start(&mSounds[soundInstanceID]);
		if (result != MA_SUCCESS)
		{
			std::cout << "Failed to start sound instance: " << soundInstanceID << std::endl;
			return false;
		}

		return true;
	}

	bool MiniAudioSubsystem::StopSoundInstance(UUID soundInstanceID)
	{
		if (mSounds.contains(soundInstanceID))
		{
			std::cout << "Sound instance was not initialized: " << soundInstanceID << std::endl;
			return false;
		}

		ma_result result;
		result = ma_sound_stop(&mSounds[soundInstanceID]);
		if (result != MA_SUCCESS)
		{
			std::cout << "Failed to stop sound instance: " << soundInstanceID << std::endl;
			return false;
		}

		return true;
	}
}
