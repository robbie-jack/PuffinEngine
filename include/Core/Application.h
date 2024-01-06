#pragma once

#include <memory>

namespace puffin::core
{
	class Engine;

	// Entry Point for Puffin Engine Apps/Games
	class Application
	{
	public:

		Application() = default;
		virtual ~Application() = default;

		virtual void setup() = 0; // Called to register application callbacks with engine

		void setEngine(const std::shared_ptr<Engine>& engine) { mEngine = engine; }

	protected:

		std::shared_ptr<Engine> mEngine = nullptr;

	};
}
