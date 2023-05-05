#include "Engine\EnkiTSSubsystem.h"

#include "Engine\Engine.h"

namespace puffin::core
{
	void EnkiTSSubsystem::setupCallbacks()
	{
		mEngine->registerCallback(core::ExecutionStage::Init, [&]() { init(); }, "EnkiTSSubsystem: Init", 50);
	}
}