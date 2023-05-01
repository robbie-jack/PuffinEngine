#include "Engine/SignalSubsystem.hpp"
#include "Engine/Engine.hpp"

namespace Puffin::Core
{
	void SignalSubsystem::SetupCallbacks()
	{
		m_engine->RegisterCallback(Core::ExecutionStage::Cleanup, [&]() { Cleanup(); }, "SignalSubsystem: Cleanup", 150);
	}
}
