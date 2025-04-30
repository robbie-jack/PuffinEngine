#pragma once

#include "puffin/core/subsystem.h"
#include "Window.hpp"

namespace puffin
{
	namespace core
	{
		class Engine;
	}

	namespace window
	{
		class WindowSubsystemRL : public core::Subsystem
		{
		public:

			explicit WindowSubsystemRL(const std::shared_ptr<core::Engine>& engine);
			~WindowSubsystemRL() override;

			void Initialize(core::SubsystemManager* subsystemManager) override;
			void Deinitialize() override;

			raylib::Window* GetWindow();

		private:

			raylib::Window* mWindow = nullptr;

		};
	}
}