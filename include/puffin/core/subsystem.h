#pragma once

#include <memory>

namespace puffin::core
{
	//////////////////////////////////////////////////
	// Subsystem
	//////////////////////////////////////////////////

	class Engine;

	/*
	 * Subsystem is a class that is responsible for some defined functionality within puffin
	 * i.e rendering, physics, window management, input, etc..
	 */
	class Subsystem
	{
	public:

		explicit Subsystem(std::shared_ptr<Engine> engine);
		virtual ~Subsystem();

		virtual void initialize();
		virtual void deinitialize();

		virtual void begin_play();
		virtual void end_play();

		/*
		 * Update method, called one a frame game is playing
		 */
		virtual void update(double delta_time);
		virtual bool should_update();

	protected:

		std::shared_ptr<Engine> m_engine = nullptr;

	};
}
