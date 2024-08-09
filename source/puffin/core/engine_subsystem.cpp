#include "puffin/core/engine_subsystem.h"

namespace puffin::core
{
	EngineSubsystem::EngineSubsystem(std::shared_ptr<Engine> engine) : Subsystem(engine)
	{

	}

	void EngineSubsystem::engine_update(double delta_time)
	{

	}

	bool EngineSubsystem::should_engine_update()
	{
		return false;
	}
}
