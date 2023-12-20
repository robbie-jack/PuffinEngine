#pragma once

#include <memory>

namespace puffin::core
{
	class Engine;

	// Subsystems manage essential low level functionality like Audio and Input
	class Subsystem
	{
	public:

		virtual ~Subsystem() { mEngine = nullptr; }

		virtual void setup() = 0; // Called to setup subsystem, for setting callbacks or registering other subsystems

		void setEngine(const std::shared_ptr<Engine>& engine)
		{
			mEngine = engine;
		}

	protected:

		std::shared_ptr<Engine> mEngine = nullptr;

	};
}