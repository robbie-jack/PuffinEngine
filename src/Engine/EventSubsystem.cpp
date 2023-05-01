#include "Engine/EventSubsystem.hpp"

#include "Engine/Engine.hpp"

namespace Puffin::Core
{
	void EventSubsystem::SetupCallbacks()
	{
		m_engine->RegisterCallback(ExecutionStage::Cleanup, [&]() { Cleanup(); }, "EventSubsystem: Cleanup", 150);
	}
}
