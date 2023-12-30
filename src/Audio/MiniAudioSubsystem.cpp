
#include "Audio/MiniAudioSubsystem.h"

#include "Core/Engine.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

namespace puffin::audio
{
	void MiniAudioSubsystem::setup()
	{
		mEngine->registerCallback(core::ExecutionStage::Init, [&]() { init(); }, "MiniAudioSubsystem: Init", 50);
		mEngine->registerCallback(core::ExecutionStage::SubsystemUpdate, [&]() { update(); }, "MiniAudioSubsystem: Update");
		mEngine->registerCallback(core::ExecutionStage::Shutdown, [&]() { shutdown(); }, "MiniAudioSubsystem: Shutdown", 150);
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
