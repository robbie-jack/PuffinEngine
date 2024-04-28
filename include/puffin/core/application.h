#pragma once

#include <memory>

namespace puffin::core
{
	class Engine;

	// Entry Point for Puffin Engine Apps/Games
	class Application
	{
	public:

		Application(const std::shared_ptr<Engine>& engine) : mEngine(engine) {}
		virtual ~Application() { mEngine = nullptr; }

	protected:

		std::shared_ptr<Engine> mEngine = nullptr;

	};
}
