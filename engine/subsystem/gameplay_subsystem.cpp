#include "subsystem/gameplay_subsystem.h"

namespace puffin::core
{
	GameplaySubsystem::GameplaySubsystem(std::shared_ptr<Engine> engine)
		: Subsystem(engine)
	{
	}

	GameplaySubsystem::~GameplaySubsystem()
	{
	}

	void GameplaySubsystem::Update(double deltaTime)
	{
	}

	bool GameplaySubsystem::ShouldUpdate()
	{
		return false;
	}

	void GameplaySubsystem::FixedUpdate(double fixedTimeStep)
	{
	}

	bool GameplaySubsystem::ShouldFixedUpdate()
	{
		return false;
	}
}
