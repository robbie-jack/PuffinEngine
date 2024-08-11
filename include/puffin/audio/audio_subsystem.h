#pragma once

#include <set>

#include "puffin/core/subsystem.h"
#include "puffin/types/uuid.h"
#include "puffin/types/packed_vector.h"

namespace puffin::audio
{
	enum class SoundEventType
	{
		None,
		Play,
		Pause,
		Stop,
		Modify
	};

	struct SoundEvent
	{
		SoundEventType type = SoundEventType::None;
		PuffinID id;
		float volume = 1.0f;
		bool looping = false;
		bool restart = false;
	};

	// Instance of a sound effect, there can be any number of instances of a particular sound
	struct SoundInstance
	{
		PuffinID instanceID = gInvalidID;
		PuffinID assetID = gInvalidID;
		bool playing = false; // Whether this instance is currently playing
		bool looping = false; // Whether this instance should loop
		float volume = 1.0f; // Volume of this instance
	};

	class AudioSubsystemProvider : public core::Subsystem
	{
	public:

		AudioSubsystemProvider(const std::shared_ptr<core::Engine>& engine);

		~AudioSubsystemProvider() override;

	protected:

		friend class AudioSubsystem;

		virtual void play_sound(PuffinID soundAssetID) = 0;

		virtual bool create_sound_instance(PuffinID soundAssetID, PuffinID soundInstanceID) = 0;
		virtual void destroy_sound_instance(PuffinID soundInstanceID) = 0;

		virtual bool start_sound_instance(PuffinID soundInstanceID, bool restart = false) = 0;
		virtual bool stop_sound_instance(PuffinID soundInstanceID) = 0;
	};

	class AudioSubsystem : public core::Subsystem
	{
	public:

		AudioSubsystem(const std::shared_ptr<core::Engine>& engine);
		~AudioSubsystem() override;

		void initialize(core::SubsystemManager* subsystem_manager) override;
		void deinitialize() override;

		void update(double delta_time) override;

		void play_sound(PuffinID soundAssetID); // Create a sound instance, play it and then immediately discard it

		PuffinID create_sound_instance(PuffinID soundAssetID); // Create sound instance for use multiple times
		void destroy_sound_instance(PuffinID soundInstanceID); // Destroy created sound instance

		void start_sound_instance(PuffinID soundInstanceID, bool restart = false); // Start sound instance playing
		void stop_sound_instance(PuffinID soundInstanceID); // Stop sound instance playing

		PuffinID create_and_start_sound_instance(PuffinID soundAssetID); // Create a new sound instance and start playing it

		const std::set<PuffinID>& get_all_instance_ids_for_sound(PuffinID soundAssetID);
		SoundInstance& get_sound_instance(PuffinID soundInstanceID); // Get sound instance struct

	private:

		friend AudioSubsystemProvider;

		PackedVector<PuffinID, SoundInstance> m_sound_instances;
		PackedVector<PuffinID, std::set<PuffinID>> m_sound_instance_ids;

		AudioSubsystemProvider* m_audio_subsystem_provider = nullptr; // Subsystem which provides audio core implementation

	};
}
