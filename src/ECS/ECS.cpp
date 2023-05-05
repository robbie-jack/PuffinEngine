#include "ECS/ECS.h"

#include "Engine/Engine.hpp"

namespace puffin::ECS
{
	void World::SetupCallbacks()
	{
		m_engine->registerCallback(core::ExecutionStage::SubsystemUpdate, [&]() { Update(); }, "ECSWorld: Update");
		m_engine->registerCallback(core::ExecutionStage::Cleanup, [&]() { Cleanup(); }, "ECSWorld: Cleanup", 150);
	}
}
