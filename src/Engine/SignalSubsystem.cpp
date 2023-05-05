#include "Engine/SignalSubsystem.hpp"
#include "Engine/Engine.hpp"

namespace puffin::core
{
	void SignalSubsystem::setupCallbacks()
	{
		mEngine->registerCallback(core::ExecutionStage::Cleanup, [&]() { cleanup(); }, "SignalSubsystem: Cleanup", 150);
	}
}
