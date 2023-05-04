#include "ECS/ECS.h"

#include "Engine/Engine.hpp"

namespace puffin::ECS
{
	void World::SetupCallbacks()
	{
		m_engine->registerCallback(Core::ExecutionStage::subsystemUpdate, [&]() { Update(); }, "ECSWorld: Update");
		m_engine->registerCallback(Core::ExecutionStage::cleanup, [&]() { Cleanup(); }, "ECSWorld: Cleanup", 150);
	}
}
