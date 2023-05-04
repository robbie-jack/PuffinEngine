#include "Engine/SignalSubsystem.hpp"
#include "Engine/Engine.hpp"

namespace puffin::Core
{
	void SignalSubsystem::SetupCallbacks()
	{
		m_engine->registerCallback(Core::ExecutionStage::cleanup, [&]() { Cleanup(); }, "SignalSubsystem: Cleanup", 150);
	}
}
