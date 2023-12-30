
#include "Audio/MiniAudioSubsystem.h"

#include "Core/Engine.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

#include <iostream>

namespace puffin::audio
{
	void MiniAudioSubsystem::setup()
	{
		mEngine->registerCallback(core::ExecutionStage::Init, [&]() { init(); }, "MiniAudioSubsystem: Init", 60);
		mEngine->registerCallback(core::ExecutionStage::SubsystemUpdate, [&]() { update(); }, "MiniAudioSubsystem: Update");
		mEngine->registerCallback(core::ExecutionStage::Shutdown, [&]() { shutdown(); }, "MiniAudioSubsystem: Shutdown", 140);
	}

	void MiniAudioSubsystem::init()
	{
		mSoundEngine = new ma_engine();

		ma_result result;
		result = ma_engine_init(NULL, mSoundEngine);
		if (result != MA_SUCCESS)
		{
			std::cout << "Failed to Initialize MiniAudio Sound Engine" << std::endl;
		}
	}

	void MiniAudioSubsystem::update()
	{

	}

	void MiniAudioSubsystem::shutdown()
	{
		if (mSoundEngine)
		{
			ma_engine_uninit(mSoundEngine);
		}
	}

	void MiniAudioSubsystem::createSoundInstanceInternal(PuffinID soundAssetID, PuffinID soundInstanceID)
	{
		
	}

	void MiniAudioSubsystem::destroySoundInstanceInternal(PuffinID soundInstanceID)
	{

	}
}
