#pragma once

#include <set>

#include "core/subsystem.h"
#include "types/uuid.h"
#include "types/storage/mapped_vector.h"

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
		UUID id;
		float volume = 1.0f;
		bool looping = false;
		bool restart = false;
	};

	// Instance of a sound effect, there can be any number of instances of a particular sound
	struct SoundInstance
	{
		UUID instanceID = gInvalidID;
		UUID assetID = gInvalidID;
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

		virtual void PlaySoundEffect(UUID soundAssetID) = 0;

		virtual bool CreateSoundInstance(UUID soundAssetID, UUID soundInstanceID) = 0;
		virtual void DestroySoundInstance(UUID soundInstanceID) = 0;

		virtual bool StartSoundInstance(UUID soundInstanceID, bool restart = false) = 0;
		virtual bool StopSoundInstance(UUID soundInstanceID) = 0;
	};

	class AudioSubsystem : public core::Subsystem
	{
	public:

		AudioSubsystem(const std::shared_ptr<core::Engine>& engine);
		~AudioSubsystem() override;

		void Initialize(core::SubsystemManager* subsystemManager) override;
		void Deinitialize() override;

		void Update(double deltaTime) override;

		void PlaySound(UUID soundAssetID); // Create a sound instance, play it and then immediately discard it

		UUID CreateSoundInstance(UUID soundAssetID); // Create sound instance for use multiple times
		void DestroySoundInstance(UUID soundInstanceID); // Destroy created sound instance

		void StartSoundInstance(UUID soundInstanceID, bool restart = false); // Start sound instance playing
		void StopSoundInstance(UUID soundInstanceID); // Stop sound instance playing

		UUID CreateAndStartSoundInstance(UUID soundAssetID); // Create a new sound instance and start playing it

		const std::set<UUID>& GetAllInstanceIDsForSound(UUID soundAssetID);
		SoundInstance& GetSoundInstance(UUID soundInstanceID); // Get sound instance struct

	private:

		friend AudioSubsystemProvider;

		MappedVector<UUID, SoundInstance> mSoundInstances;
		MappedVector<UUID, std::set<UUID>> mSoundInstanceIDs;

		AudioSubsystemProvider* mAudioSubsystemProvider = nullptr; // Subsystem which provides audio core implementation

	};
}
