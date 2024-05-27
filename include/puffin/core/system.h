#pragma once

#include <memory>

namespace puffin::core
{
	//////////////////////////////////////////////////
	// System
	//////////////////////////////////////////////////

	class Engine;

	class System
	{
	public:

		System(const std::shared_ptr<Engine>& engine) { m_engine = engine; }

		virtual ~System() { m_engine = nullptr; }

	protected:

		std::shared_ptr<Engine> m_engine = nullptr;

	};
}
