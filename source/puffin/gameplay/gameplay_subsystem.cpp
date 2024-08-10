#include "puffin/gameplay/gameplay_subsystem.h"

namespace puffin::gameplay
{
	GameplaySubsystem::GameplaySubsystem(std::shared_ptr<core::Engine> engine) : Subsystem(engine)
	{
	}

	void GameplaySubsystem::fixed_update(double fixed_time)
	{
	}

	bool GameplaySubsystem::should_fixed_update()
	{
		return false;
	}
}
