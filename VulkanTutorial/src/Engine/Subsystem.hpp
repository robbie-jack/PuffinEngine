#pragma once

#include <memory>

namespace Puffin::Core
{
	typedef uint8_t SubsystemID;

	class Engine;

	// Subsystems manage essential low level functionality like Audio and Input
	class Subsystem
	{
	public:

		virtual ~Subsystem() { m_engine = nullptr; }

		bool ShouldUpdate() const
		{
			return m_shouldUpdate;
		}

		virtual void Init() = 0;
		virtual void Update() = 0;
		virtual void Destroy() = 0;

		void SetEngine(std::shared_ptr<Engine> engine)
		{
			m_engine = engine;
		}

	protected:

		bool m_shouldUpdate = false;;
		std::shared_ptr<Engine> m_engine = nullptr;

	};
}