#include "subsystem/engine_subsystem.h"

namespace puffin::core
{
	EngineSubsystem::EngineSubsystem(std::shared_ptr<Engine> engine)
		: Subsystem(engine)
	{
	}

	EngineSubsystem::~EngineSubsystem()
	{
	}

	void EngineSubsystem::Update(double deltaTime)
	{
	}

	bool EngineSubsystem::ShouldUpdate()
	{
		return false;
	}
}
