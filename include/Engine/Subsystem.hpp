#pragma once

#include <memory>

namespace Puffin::Core
{
	class Engine;

	// Subsystems manage essential low level functionality like Audio and Input
	class Subsystem
	{
	public:

		virtual ~Subsystem() { m_engine = nullptr; }

		virtual void SetupCallbacks() = 0; // Called to setup this systems callbacks when it is registered

		void SetEngine(std::shared_ptr<Engine> engine)
		{
			m_engine = engine;
		}

	protected:

		std::shared_ptr<Engine> m_engine = nullptr;

	};
}