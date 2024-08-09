#pragma once

#include "puffin/core/subsystem.h"

namespace puffin::core
{
	/*
	 * Subsystem which shares lifetime of the engine
	 */
	class EngineSubsystem : public Subsystem
	{
	public:

		explicit EngineSubsystem(std::shared_ptr<Engine> engine);
		~EngineSubsystem() override = default;
	
	};
}