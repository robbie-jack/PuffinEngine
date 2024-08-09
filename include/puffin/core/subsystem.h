#pragma once

#include <memory>

namespace puffin::core
{
	//////////////////////////////////////////////////
	// Subsystem
	//////////////////////////////////////////////////

	class Engine;

	class Subsystem
	{
	public:

		explicit Subsystem(std::shared_ptr<Engine> engine);
		virtual ~Subsystem();

		virtual void initialize();
		virtual void deinitialize();

		virtual void begin_play();
		virtual void end_play();

		virtual void update(double delta_time);
		virtual bool should_update();

		virtual void fixed_update(double fixed_time);
		virtual bool should_fixed_update();

		virtual void render(double delta_time);
		virtual bool should_render();

	protected:

		std::shared_ptr<Engine> m_engine = nullptr;

	};
}
