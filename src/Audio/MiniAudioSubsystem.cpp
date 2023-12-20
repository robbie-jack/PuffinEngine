
#include "Audio/MiniAudioSubsystem.h"

#include "Core/Engine.h"

namespace puffin::audio
{
	void MiniAudioSubsystem::setup()
	{
		mEngine->registerCallback(core::ExecutionStage::Init, [&]() { init(); }, "AudioSubsystem: Init", 50);
		mEngine->registerCallback(core::ExecutionStage::SubsystemUpdate, [&]() { update(); }, "AudioSubsystem: Update");
		mEngine->registerCallback(core::ExecutionStage::Shutdown, [&]() { shutdown(); }, "AudioSubsystem: Shutdown", 150);
	}

	void MiniAudioSubsystem::init()
	{

	}

	void MiniAudioSubsystem::update()
	{

	}

	void MiniAudioSubsystem::shutdown()
	{

	}
}
