#pragma once

#include "irrKlang/irrKlang.h"

#include "Core/Subsystem.h"

#include "Types/UUID.h"
#include "Types/RingBuffer.h"
#include "Types/PackedArray.h"

#include <set>

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
		bool discard = true; // Whether to discard this instance once it has finished playing, will be ignored if looping is set to true
		float volume = 1.0f; // Volume of this instance
	};

	class AudioSubsystemProvider : public core::Subsystem
	{
	public:

		~AudioSubsystemProvider() override { mEngine = nullptr; }

		PuffinID createSoundInstance(PuffinID soundAssetID);

		void destroySoundInstance(PuffinID soundInstanceID);

	protected:

		PackedVector<SoundInstance> mSoundInstances;
		PackedVector<std::set<PuffinID>> mSoundInstanceIDs;

		virtual void createSoundInstanceInternal(PuffinID soundAssetID, PuffinID soundInstanceID) = 0;

		virtual void destroySoundInstanceInternal(PuffinID soundInstanceID) = 0;

	};

	class AudioSubsystem : public core::Subsystem
	{
	public:

		AudioSubsystem() = default;
		~AudioSubsystem() override = default;

		void setup() override;

		void init();
		void update();
		void shutdown();

	private:

		std::shared_ptr<AudioSubsystemProvider> mAudioSubsystemProvider = nullptr; // Susbsystem which provides audio core implementation

	};
}
