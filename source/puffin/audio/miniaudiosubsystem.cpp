#include "puffin/audio/mini_audio_subsystem.h"

#define MINIAUDIO_IMPLEMENTATION
#include "puffin/audio/mini_audio_subsystem.h"

#include "miniaudio/miniaudio.h"

#include <iostream>

#include "puffin/core/engine.h"
#include "puffin/assets/asset_registry.h"
#include "puffin/assets/sound_asset.h"
#include "puffin/audio/audio_subsystem.h"

namespace puffin::audio
{
	MiniAudioSubsystem::MiniAudioSubsystem(const std::shared_ptr<core::Engine>& engine) : audio::AudioSubsystemProvider(engine)
	{
	}

	MiniAudioSubsystem::~MiniAudioSubsystem()
	{
		m_engine = nullptr;
	}

	void MiniAudioSubsystem::initialize(core::SubsystemManager* subsystem_manager)
	{
		AudioSubsystemProvider::initialize(subsystem_manager);

		m_sound_engine = new ma_engine();

		if (const ma_result result = ma_engine_init(nullptr, m_sound_engine); result != MA_SUCCESS)
		{
			std::cout << "Failed to Initialize MiniAudio Sound Engine" << std::endl;
		}
	}

	void MiniAudioSubsystem::deinitialize()
	{
		AudioSubsystemProvider::deinitialize();

		for (auto& sound : m_sounds)
		{
			ma_sound_uninit(&sound);
		}

		m_sounds.clear();

		if (m_sound_engine)
		{
			ma_engine_uninit(m_sound_engine);
		}
	}

	void MiniAudioSubsystem::update(double delta_time)
	{
		AudioSubsystemProvider::update(delta_time);
	}

	void MiniAudioSubsystem::play_sound(PuffinID sound_asset_id)
	{
		auto soundAsset = assets::AssetRegistry::get()->get_asset<assets::SoundAsset>(sound_asset_id);
		auto soundPath = (assets::AssetRegistry::get()->content_root() / soundAsset->relativePath()).string();

		ma_engine_play_sound(m_sound_engine, soundPath.c_str(), nullptr);
	}

	bool MiniAudioSubsystem::create_sound_instance(PuffinID sound_asset_id, PuffinID sound_instance_id)
	{
		if (m_sounds.contains(sound_instance_id))
		{
			return true;
		}

		auto soundAsset = assets::AssetRegistry::get()->get_asset<assets::SoundAsset>(sound_asset_id);
		auto soundPath = (assets::AssetRegistry::get()->content_root() / soundAsset->relativePath()).string();

		m_sounds.emplace(sound_instance_id, ma_sound());

		ma_result result;
		result = ma_sound_init_from_file(m_sound_engine, soundPath.c_str(), 0, nullptr, nullptr, &m_sounds[sound_instance_id]);
		if (result != MA_SUCCESS)
		{
			std::cout << "Failed to initialize sound instance for: " << soundPath << std::endl;
			return false;
		}

		return true;
	}

	void MiniAudioSubsystem::destroy_sound_instance(PuffinID sound_instance_id)
	{
		if (!m_sounds.contains(sound_instance_id))
		{
			return;
		}

		ma_sound_uninit(&m_sounds[sound_instance_id]);
		m_sounds.erase(sound_instance_id);
	}

	bool MiniAudioSubsystem::start_sound_instance(PuffinID sound_instance_id, bool restart)
	{
		if (m_sounds.contains(sound_instance_id))
		{
			std::cout << "Sound instance was not initialized: " << sound_instance_id << std::endl;
			return false;
		}

		ma_result result;
		if (restart)
		{
			result = ma_sound_seek_to_pcm_frame(&m_sounds[sound_instance_id], 0);
			if (result != MA_SUCCESS)
			{
				std::cout << "Failed to restart sound instance: " << sound_instance_id << std::endl;
				return false;
			}
		}
				
		result = ma_sound_start(&m_sounds[sound_instance_id]);
		if (result != MA_SUCCESS)
		{
			std::cout << "Failed to start sound instance: " << sound_instance_id << std::endl;
			return false;
		}

		return true;
	}

	bool MiniAudioSubsystem::stop_sound_instance(PuffinID sound_instance_id)
	{
		if (m_sounds.contains(sound_instance_id))
		{
			std::cout << "Sound instance was not initialized: " << sound_instance_id << std::endl;
			return false;
		}

		ma_result result;
		result = ma_sound_stop(&m_sounds[sound_instance_id]);
		if (result != MA_SUCCESS)
		{
			std::cout << "Failed to stop sound instance: " << sound_instance_id << std::endl;
			return false;
		}

		return true;
	}
}
