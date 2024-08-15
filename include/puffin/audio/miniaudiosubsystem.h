#pragma once

#include "audio_subsystem.h"

#include "miniaudio/miniaudio.h"

namespace puffin::audio
{
	class MiniAudioSubsystem : public AudioSubsystemProvider
	{
	public:

		MiniAudioSubsystem(const std::shared_ptr<core::Engine>& engine);
		~MiniAudioSubsystem() override;

		void initialize(core::SubsystemManager* subsystem_manager) override;
		void deinitialize() override;

		void update(double delta_time) override;

	protected:

		void play_sound(PuffinID sound_asset_id) override;

		bool create_sound_instance(PuffinID sound_asset_id, PuffinID sound_instance_id) override;
		void destroy_sound_instance(PuffinID sound_instance_id) override;

		bool start_sound_instance(PuffinID sound_instance_id, bool restart) override;
		bool stop_sound_instance(PuffinID sound_instance_id) override;

	private:

		ma_engine* m_sound_engine = nullptr;

		PackedVector<PuffinID, ma_sound> m_sounds;
	};
}
