#include "Core/EnkiTSSubsystem.h"

#include "Core/Engine.h"

namespace puffin::core
{
	void EnkiTSSubsystem::setup()
	{
		mEngine->registerCallback(core::ExecutionStage::Init, [&]() { init(); }, "EnkiTSSubsystem: Init", 50);
	}
}