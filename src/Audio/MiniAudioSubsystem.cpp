
#include "Audio/MiniAudioSubsystem.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

#include "Core/Engine.h"
#include "Assets/AssetRegistry.h"
#include "Assets/SoundAsset.h"

#include <iostream>

namespace puffin::audio
{
	MiniAudioSubsystem::MiniAudioSubsystem(const std::shared_ptr<core::Engine>& engine) : AudioSubsystemProvider(engine)
	{
		mEngine->registerCallback(core::ExecutionStage::Startup, [&]() { startup(); }, "MiniAudioSubsystem: Startup", 60);
		mEngine->registerCallback(core::ExecutionStage::SubsystemUpdate, [&]() { update(); }, "MiniAudioSubsystem: Update");
		mEngine->registerCallback(core::ExecutionStage::Shutdown, [&]() { shutdown(); }, "MiniAudioSubsystem: Shutdown", 140);
	}

	void MiniAudioSubsystem::startup()
	{
		mSoundEngine = new ma_engine();

		ma_result result;
		result = ma_engine_init(nullptr, mSoundEngine);
		if (result != MA_SUCCESS)
		{
			std::cout << "Failed to Initialize MiniAudio Sound Engine" << std::endl;
		}
	}

	void MiniAudioSubsystem::update()
	{

	}

	void MiniAudioSubsystem::shutdown()
	{
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

	void MiniAudioSubsystem::playSound(PuffinID soundAssetID)
	{
		auto soundAsset = assets::AssetRegistry::get()->getAsset<assets::SoundAsset>(soundAssetID);
		auto soundPath = (assets::AssetRegistry::get()->contentRoot() / soundAsset->relativePath()).string();

		ma_engine_play_sound(mSoundEngine, soundPath.c_str(), nullptr);
	}

	bool MiniAudioSubsystem::createSoundInstance(PuffinID soundAssetID, PuffinID soundInstanceID)
	{
		if (mSounds.contains(soundInstanceID))
		{
			return true;
		}

		auto soundAsset = assets::AssetRegistry::get()->getAsset<assets::SoundAsset>(soundAssetID);
		auto soundPath = (assets::AssetRegistry::get()->contentRoot() / soundAsset->relativePath()).string();

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

	void MiniAudioSubsystem::destroySoundInstance(PuffinID soundInstanceID)
	{
		if (!mSounds.contains(soundInstanceID))
		{
			return;
		}

		ma_sound_uninit(&mSounds[soundInstanceID]);
		mSounds.erase(soundInstanceID);
	}

	bool MiniAudioSubsystem::startSoundInstance(PuffinID soundInstanceID, bool restart)
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

	bool MiniAudioSubsystem::stopSoundInstance(PuffinID soundInstanceID)
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
