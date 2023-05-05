#include "Engine/SignalSubsystem.hpp"
#include "Engine/Engine.hpp"

namespace puffin::core
{
	void SignalSubsystem::SetupCallbacks()
	{
		m_engine->registerCallback(core::ExecutionStage::Cleanup, [&]() { Cleanup(); }, "SignalSubsystem: Cleanup", 150);
	}
}
