#include "puffin/audio/audio_subsystem.h"

#include <memory>

#include "puffin/core/engine.h"
#include "puffin/audio/mini_audio_subsystem.h"

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
		m_engine = nullptr;
	}

	////////////////////////////////
	// AudioSubsystem
	////////////////////////////////

	AudioSubsystem::AudioSubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
		
	}

	AudioSubsystem::~AudioSubsystem()
	{
		m_engine = nullptr;
	}

	void AudioSubsystem::initialize(core::ISubsystemManager* subsystem_manager)
	{
		Subsystem::initialize(subsystem_manager);

		m_audio_subsystem_provider = new MiniAudioSubsystem(m_engine);
		m_audio_subsystem_provider->initialize(subsystem_manager);
	}

	void AudioSubsystem::deinitialize()
	{
		Subsystem::deinitialize();

		m_audio_subsystem_provider->deinitialize();

		delete m_audio_subsystem_provider;
		m_audio_subsystem_provider = nullptr;
	}

	void AudioSubsystem::update(double delta_time)
	{
		
	}

	void AudioSubsystem::play_sound(PuffinID soundAssetID)
	{
		m_audio_subsystem_provider->play_sound(soundAssetID);
	}

	PuffinID AudioSubsystem::create_sound_instance(PuffinID soundAssetID)
	{
		const PuffinID soundInstanceID = generate_id();

		if (!m_audio_subsystem_provider->create_sound_instance(soundAssetID, soundInstanceID))
		{
			return gInvalidID;
		}

		m_sound_instances.emplace(soundInstanceID, SoundInstance());

		m_sound_instances[soundInstanceID].instanceID = soundInstanceID;
		m_sound_instances[soundInstanceID].assetID = soundAssetID;

		m_sound_instance_ids[soundAssetID].insert(soundInstanceID);

		return soundInstanceID;
	}

	void AudioSubsystem::destroy_sound_instance(PuffinID soundInstanceID)
	{
		const PuffinID soundAssetID = m_sound_instances[soundInstanceID].assetID;

		m_sound_instances.erase(soundInstanceID);
		m_sound_instance_ids[soundAssetID].erase(soundInstanceID);
	}

	void AudioSubsystem::start_sound_instance(PuffinID soundInstanceID, bool restart)
	{
		m_audio_subsystem_provider->start_sound_instance(soundInstanceID, restart);
	}

	void AudioSubsystem::stop_sound_instance(PuffinID soundInstanceID)
	{
		m_audio_subsystem_provider->stop_sound_instance(soundInstanceID);
	}

	PuffinID AudioSubsystem::create_and_start_sound_instance(PuffinID soundAssetID)
	{
		const PuffinID soundInstanceID = create_sound_instance(soundAssetID);

		start_sound_instance(soundInstanceID);

		return soundInstanceID;
	}

	const std::set<PuffinID>& AudioSubsystem::get_all_instance_ids_for_sound(PuffinID soundAssetID)
	{
		return m_sound_instance_ids[soundAssetID];
	}

	SoundInstance& AudioSubsystem::get_sound_instance(PuffinID soundInstanceID)
	{
		return m_sound_instances[soundInstanceID];
	}
}
