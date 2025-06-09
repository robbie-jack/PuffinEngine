#pragma once

#include "window/window_subsystem.h"
#include "Window.hpp"

namespace puffin
{
	namespace core
	{
		class Engine;
	}

	namespace window
	{
		class RaylibWindowSubsystem : public WindowSubsystem
		{
		public:

			explicit RaylibWindowSubsystem(const std::shared_ptr<core::Engine>& engine);
			~RaylibWindowSubsystem() override;

			void Initialize(core::SubsystemManager* subsystemManager) override;
			void Deinitialize() override;

		private:

			

		};
	}
}