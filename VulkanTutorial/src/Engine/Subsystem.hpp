#pragma once

#include <memory>

namespace Puffin::Core
{
	typedef uint8_t SubsystemID;

	class Engine;

	// Subsystems manage
	class Subsystem
	{
	public:

		virtual ~Subsystem() = default;

		virtual bool ShouldUpdate() const
		{
			return false;
		}

		virtual void Init() = 0;
		virtual void Update() = 0;
		virtual void Destroy() = 0;

		void SetEngine(std::shared_ptr<Engine> engine)
		{
			m_engine = engine;
		}

	protected:

		std::shared_ptr<Engine> m_engine = nullptr;

	};
}