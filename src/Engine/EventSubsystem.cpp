#include "Engine/EventSubsystem.hpp"

#include "Engine/Engine.hpp"

namespace Puffin::Core
{
	void EventSubsystem::SetupCallbacks()
	{
		m_engine->registerCallback(ExecutionStage::cleanup, [&]() { Cleanup(); }, "EventSubsystem: Cleanup", 150);
	}
}
