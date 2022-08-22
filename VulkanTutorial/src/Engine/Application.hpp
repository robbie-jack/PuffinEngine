#pragma once

#include "Engine/Engine.hpp"

namespace Puffin::Core
{
	// Entry Point for Puffin Engine Apps/Games
	class Application
	{
	public:

		Application(std::shared_ptr<Engine> engine) : m_engine(engine) {}
		virtual ~Application() = default;

		virtual void Init() = 0;
		virtual void Update() = 0;
		virtual void Destroy() = 0;

	private:

		std::shared_ptr<Engine> m_engine = nullptr;

	};
}