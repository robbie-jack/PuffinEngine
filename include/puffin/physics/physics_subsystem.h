#pragma once

#include "puffin/gameplay/gameplay_subsystem.h"

namespace puffin::physics
{
	/*
	 * Gameplay subsystem child which can execute a fixed_update method at set physics tick
	 * To be used for physics related code, like 3rd party physics engine integrations
	 */
	class PhysicsSubsystem : public gameplay::GameplaySubsystem
	{
	public:

		PhysicsSubsystem(std::shared_ptr<core::Engine> engine);
		~PhysicsSubsystem() override = default;

		virtual void fixed_update(double fixed_time);

	};
}