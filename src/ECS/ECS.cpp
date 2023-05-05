#include "ECS/ECS.h"

#include "Engine\Engine.h"

namespace puffin::ECS
{
	void World::setupCallbacks()
	{
		mEngine->registerCallback(core::ExecutionStage::SubsystemUpdate, [&]() { Update(); }, "ECSWorld: Update");
		mEngine->registerCallback(core::ExecutionStage::Cleanup, [&]() { Cleanup(); }, "ECSWorld: Cleanup", 150);
	}
}
