#pragma once

#include <set>

#include "puffin/core/system.h"
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

	class AudioSubsystemProvider : public core::System
	{
	public:

		AudioSubsystemProvider(const std::shared_ptr<core::Engine>& engine) : System(engine) {}

		~AudioSubsystemProvider() override { m_engine = nullptr; }

	protected:

		friend class AudioSubsystem;

		virtual void playSound(PuffinID soundAssetID) = 0;

		virtual bool createSoundInstance(PuffinID soundAssetID, PuffinID soundInstanceID) = 0;
		virtual void destroySoundInstance(PuffinID soundInstanceID) = 0;

		virtual bool startSoundInstance(PuffinID soundInstanceID, bool restart = false) = 0;
		virtual bool stopSoundInstance(PuffinID soundInstanceID) = 0;
	};

	class AudioSubsystem : public core::System
	{
	public:

		AudioSubsystem(const std::shared_ptr<core::Engine>& engine);
		~AudioSubsystem() override { m_engine = nullptr; }

		void startup();
		void update();
		void shutdown();

		void playSound(PuffinID soundAssetID); // Create a sound instance, play it and then immediately discard it

		PuffinID createSoundInstance(PuffinID soundAssetID); // Create sound instance for use multiple times
		void destroySoundInstance(PuffinID soundInstanceID); // Destroy created sound instance

		void startSoundInstance(PuffinID soundInstanceID, bool restart = false); // Start sound instance playing
		void stopSoundInstance(PuffinID soundInstanceID); // Stop sound instance playing

		PuffinID createAndStartSoundInstance(PuffinID soundAssetID); // Create a new sound instance and start playing it

		const std::set<PuffinID>& getAllInstanceIDsForSound(PuffinID soundAssetID);
		SoundInstance& getSoundInstance(PuffinID soundInstanceID); // Get sound instance struct

	private:

		friend AudioSubsystemProvider;

		PackedVector<PuffinID, SoundInstance> mSoundInstances;
		PackedVector<PuffinID, std::set<PuffinID>> mSoundInstanceIDs;

		std::shared_ptr<AudioSubsystemProvider> mAudioSubsystemProvider = nullptr; // Subsystem which provides audio core implementation

	};
}
