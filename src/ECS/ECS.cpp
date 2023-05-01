#include "ECS/ECS.h"

#include "Engine/Engine.hpp"

namespace Puffin::ECS
{
	void World::SetupCallbacks()
	{
		m_engine->RegisterCallback(Core::ExecutionStage::SubsystemUpdate, [&]() { Update(); }, "ECSWorld: Update");
		m_engine->RegisterCallback(Core::ExecutionStage::Cleanup, [&]() { Cleanup(); }, "ECSWorld: Cleanup", 150);
	}
}
