#pragma once

#include "Core/Subsystem.h"

namespace puffin::audio
{
	class MiniAudioSubsystem : public core::Subsystem
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
