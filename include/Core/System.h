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

		System(const std::shared_ptr<Engine>& engine) { mEngine = engine; }

		virtual ~System() { mEngine = nullptr; }

	protected:

		std::shared_ptr<Engine> mEngine = nullptr;

	};
}
