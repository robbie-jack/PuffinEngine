#pragma once

#include <set>

#include "puffin/core/subsystem.h"
#include "puffin/types/uuid.h"
#include "puffin/types/packedvector.h"

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

		virtual void PlaySoundEffect(PuffinID soundAssetID) = 0;

		virtual bool CreateSoundInstance(PuffinID soundAssetID, PuffinID soundInstanceID) = 0;
		virtual void DestroySoundInstance(PuffinID soundInstanceID) = 0;

		virtual bool StartSoundInstance(PuffinID soundInstanceID, bool restart = false) = 0;
		virtual bool StopSoundInstance(PuffinID soundInstanceID) = 0;
	};

	class AudioSubsystem : public core::Subsystem
	{
	public:

		AudioSubsystem(const std::shared_ptr<core::Engine>& engine);
		~AudioSubsystem() override;

		void Initialize(core::SubsystemManager* subsystemManager) override;
		void Deinitialize() override;

		void Update(double deltaTime) override;

		void PlaySound(PuffinID soundAssetID); // Create a sound instance, play it and then immediately discard it

		PuffinID CreateSoundInstance(PuffinID soundAssetID); // Create sound instance for use multiple times
		void DestroySoundInstance(PuffinID soundInstanceID); // Destroy created sound instance

		void StartSoundInstance(PuffinID soundInstanceID, bool restart = false); // Start sound instance playing
		void StopSoundInstance(PuffinID soundInstanceID); // Stop sound instance playing

		PuffinID CreateAndStartSoundInstance(PuffinID soundAssetID); // Create a new sound instance and start playing it

		const std::set<PuffinID>& GetAllInstanceIDsForSound(PuffinID soundAssetID);
		SoundInstance& GetSoundInstance(PuffinID soundInstanceID); // Get sound instance struct

	private:

		friend AudioSubsystemProvider;

		PackedVector<PuffinID, SoundInstance> mSoundInstances;
		PackedVector<PuffinID, std::set<PuffinID>> mSoundInstanceIDs;

		AudioSubsystemProvider* mAudioSubsystemProvider = nullptr; // Subsystem which provides audio core implementation

	};
}
