#include "Core/SignalSubsystem.h"
#include "Core/Engine.h"

namespace puffin::core
{
	void SignalSubsystem::setup()
	{
		mEngine->registerCallback(core::ExecutionStage::Shutdown, [&]() { shutdown(); }, "SignalSubsystem: Shutdown", 150);
	}
}
