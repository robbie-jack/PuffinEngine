#pragma once

#include <memory>

namespace puffin::Core
{
	class Engine;

	// Entry Point for Puffin Engine Apps/Games
	class Application
	{
	public:

		Application() = default;
		virtual ~Application() = default;

		virtual void Init() = 0;
		virtual void Start() = 0;
		virtual void Update() = 0;
		virtual void Stop() = 0;
		virtual void Destroy() = 0;

		void SetEngine(std::shared_ptr<Engine> engine) { m_engine = engine; }

	private:

		std::shared_ptr<Engine> m_engine = nullptr;

	};
}
