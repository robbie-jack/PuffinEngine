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

		/*
		 * Engine update method, called once a frame whether game is playing or not,
		 * called before gameplay update method
		 */
		virtual void engine_update(double delta_time);
		virtual bool should_engine_update();
	
	};
}