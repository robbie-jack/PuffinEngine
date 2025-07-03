#define MINIAUDIO_IMPLEMENTATION
#include "audio/mini_audio_subsystem.h"

#include "miniaudio/miniaudio.h"

#include <iostream>

#include "core/engine.h"
#include "audio/audio_subsystem.h"

namespace puffin::audio
{
	MiniAudioSubsystem::MiniAudioSubsystem(const std::shared_ptr<core::Engine>& engine)
		: audio::AudioSubsystemProvider(engine)
	{
	}

	MiniAudioSubsystem::~MiniAudioSubsystem()
	{
		m_engine = nullptr;
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

		mSounds.Clear();

		if (mSoundEngine)
		{
			ma_engine_uninit(mSoundEngine);
		}
	}

	void MiniAudioSubsystem::Update(double deltaTime)
	{
		AudioSubsystemProvider::Update(deltaTime);
	}

	std::string_view MiniAudioSubsystem::GetName() const
	{
		return reflection::GetTypeString<MiniAudioSubsystem>();
	}

	void MiniAudioSubsystem::PlaySoundEffect(UUID soundAssetID)
	{
		// PUFFIN_TODO - Reimplement after implementing new resource system
		//auto soundAsset = assets::AssetRegistry::Get()->GetAsset<assets::SoundAsset>(soundAssetID);
		//auto soundPath = (assets::AssetRegistry::Get()->GetContentRoot() / soundAsset->GetRelativePath()).string();

		//ma_engine_play_sound(mSoundEngine, soundPath.c_str(), nullptr);
	}

	bool MiniAudioSubsystem::CreateSoundInstance(UUID soundAssetID, UUID soundInstanceID)
	{
		if (mSounds.Contains(soundInstanceID))
		{
			return true;
		}

		// PUFFIN_TODO - Reimplement after implementing new resource system
		//auto soundAsset = assets::AssetRegistry::Get()->GetAsset<assets::SoundAsset>(soundAssetID);
		//auto soundPath = (assets::AssetRegistry::Get()->GetContentRoot() / soundAsset->GetRelativePath()).string();

		/*mSounds.Emplace(soundInstanceID, ma_sound());

		ma_result result;
		result = ma_sound_init_from_file(mSoundEngine, soundPath.c_str(), 0, nullptr, nullptr, &mSounds[soundInstanceID]);
		if (result != MA_SUCCESS)
		{
			std::cout << "Failed to initialize sound instance for: " << soundPath << std::endl;
			return false;
		}*/

		return true;
	}

	void MiniAudioSubsystem::DestroySoundInstance(UUID soundInstanceID)
	{
		if (!mSounds.Contains(soundInstanceID))
		{
			return;
		}

		ma_sound_uninit(&mSounds[soundInstanceID]);
		mSounds.Erase(soundInstanceID);
	}

	bool MiniAudioSubsystem::StartSoundInstance(UUID soundInstanceID, bool restart)
	{
		if (mSounds.Contains(soundInstanceID))
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
		if (mSounds.Contains(soundInstanceID))
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
