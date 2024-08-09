#pragma once

#include "puffin/core/subsystem.h"

namespace puffin::gameplay
{
	/*
	 * Subsystem which is created when gameplay starts, and is destroyed when gameplay ends
	 */
	class GameplaySubsystem : public core::Subsystem
	{
	public:

		explicit GameplaySubsystem(std::shared_ptr<core::Engine> engine);
		~GameplaySubsystem() override = default;

	};
}