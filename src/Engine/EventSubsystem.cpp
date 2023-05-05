#include "Engine/EventSubsystem.hpp"

#include "Engine/Engine.hpp"

namespace puffin::core
{
	void EventSubsystem::SetupCallbacks()
	{
		m_engine->registerCallback(ExecutionStage::Cleanup, [&]() { Cleanup(); }, "EventSubsystem: Cleanup", 150);
	}
}
