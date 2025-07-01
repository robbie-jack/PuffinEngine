#pragma once

#include <set>

#include "subsystem/engine_subsystem.h"
#include "types/uuid.h"
#include "types/storage/mapped_vector.h"

namespace puffin
{
	namespace audio
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

		class AudioSubsystemProvider : public core::EngineSubsystem
		{
		public:

			AudioSubsystemProvider(const std::shared_ptr<core::Engine>& engine);
			~AudioSubsystemProvider() override;

			std::string_view GetName() const override;

		protected:

			friend class AudioSubsystem;

			virtual void PlaySoundEffect(UUID soundAssetID) = 0;

			virtual bool CreateSoundInstance(UUID soundAssetID, UUID soundInstanceID) = 0;
			virtual void DestroySoundInstance(UUID soundInstanceID) = 0;

			virtual bool StartSoundInstance(UUID soundInstanceID, bool restart = false) = 0;
			virtual bool StopSoundInstance(UUID soundInstanceID) = 0;
		};

		class AudioSubsystem : public core::EngineSubsystem
		{
		public:

			AudioSubsystem(const std::shared_ptr<core::Engine>& engine);
			~AudioSubsystem() override;

			void PreInitialize(core::SubsystemManager* subsystemManager) override;
			void Initialize() override;
			void Deinitialize() override;

			void Update(double deltaTime) override;

			std::string_view GetName() const override;

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

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<audio::AudioSubsystemProvider>()
		{
			return "AudioSubsystemProvider";
		}

		template<>
		inline entt::hs GetTypeHashedString<audio::AudioSubsystemProvider>()
		{
			return entt::hs(GetTypeString<audio::AudioSubsystemProvider>().data());
		}

		template<>
		inline void RegisterType<audio::AudioSubsystemProvider>()
		{
			auto meta = entt::meta<audio::AudioSubsystemProvider>()
				.base<core::EngineSubsystem>()
				.base<core::Subsystem>();

			RegisterTypeDefaults(meta);
			//RegisterSubsystemDefault(meta);
		}

		template<>
		inline std::string_view GetTypeString<audio::AudioSubsystem>()
		{
			return "AudioSubsystem";
		}

		template<>
		inline entt::hs GetTypeHashedString<audio::AudioSubsystem>()
		{
			return entt::hs(GetTypeString<audio::AudioSubsystem>().data());
		}

		template<>
		inline void RegisterType<audio::AudioSubsystem>()
		{
			auto meta = entt::meta<audio::AudioSubsystem>()
				.base<core::EngineSubsystem>()
				.base<core::Subsystem>();

			RegisterTypeDefaults(meta);
			RegisterSubsystemDefault(meta);
		}
	}
}
