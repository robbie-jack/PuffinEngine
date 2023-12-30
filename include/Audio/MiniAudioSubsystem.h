#pragma once

#include "Core/Subsystem.h"
#include "Audio/AudioSubsystem.h"

namespace puffin::audio
{
	class MiniAudioSubsystem : public IAudioSubsystem
	{
	public:

		MiniAudioSubsystem() = default;
		~MiniAudioSubsystem() override = default;

		void setup() override;

		void init();
		void update();
		void shutdown();
	};
}
