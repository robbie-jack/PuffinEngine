#pragma once

#include <miniaudio/miniaudio.h>

#include "audio/audio_subsystem.h"
#include "types/storage/mapped_vector.h"

namespace puffin
{
	namespace audio
	{
		class MiniAudioSubsystem : public AudioSubsystemProvider
		{
		public:

			MiniAudioSubsystem(const std::shared_ptr<core::Engine>& engine);
			~MiniAudioSubsystem() override;

			void Initialize(core::SubsystemManager* subsystemManager) override;
			void Deinitialize() override;

			void Update(double deltaTime) override;

			std::string_view GetName() const override;

		protected:

			void PlaySoundEffect(UUID soundAssetID) override;

			bool CreateSoundInstance(UUID soundAssetID, UUID soundInstanceID) override;
			void DestroySoundInstance(UUID soundInstanceID) override;

			bool StartSoundInstance(UUID soundInstanceID, bool restart) override;
			bool StopSoundInstance(UUID soundInstanceID) override;

		private:

			ma_engine* mSoundEngine = nullptr;

			MappedVector<UUID, ma_sound> mSounds;
		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<audio::MiniAudioSubsystem>()
		{
			return "MiniAudioSubsystem";
		}

		template<>
		inline entt::hs GetTypeHashedString<audio::MiniAudioSubsystem>()
		{
			return entt::hs(GetTypeString<audio::MiniAudioSubsystem>().data());
		}

		template<>
		inline void RegisterType<audio::MiniAudioSubsystem>()
		{
			auto meta = entt::meta<audio::MiniAudioSubsystem>()
				.base<audio::AudioSubsystemProvider>()
				.base<core::EngineSubsystem>()
				.base<core::Subsystem>();

			RegisterTypeDefaults(meta);
			RegisterSubsystemDefault(meta);
		}
	}
}
