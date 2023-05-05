#include "Engine\SignalSubsystem.h"
#include "Engine\Engine.h"

namespace puffin::core
{
	void SignalSubsystem::setupCallbacks()
	{
		mEngine->registerCallback(core::ExecutionStage::Cleanup, [&]() { cleanup(); }, "SignalSubsystem: Cleanup", 150);
	}
}
