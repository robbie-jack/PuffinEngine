#pragma once

#include "puffin/window/windowsubsystem.h"
#include "Window.hpp"

namespace puffin
{
	namespace core
	{
		class Engine;
	}

	namespace window
	{
		class WindowSubsystemRL : public WindowSubsystem
		{
		public:

			explicit WindowSubsystemRL(const std::shared_ptr<core::Engine>& engine);
			~WindowSubsystemRL() override;

			void Initialize(core::SubsystemManager* subsystemManager) override;
			void Deinitialize() override;

		private:

			

		};
	}
}