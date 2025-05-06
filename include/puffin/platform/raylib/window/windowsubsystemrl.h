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

			[[nodiscard]] bool ShouldPrimaryWindowClose() const override;

			[[nodiscard]] Size GetPrimaryWindowSize() const override;
			[[nodiscard]] uint32_t GetPrimaryWindowWidth() const override;
			[[nodiscard]] uint32_t GetPrimaryWindowHeight() const override;

			raylib::Window* GetPrimaryWindow();

		private:

			raylib::Window* mPrimaryWindow = nullptr;

		};
	}
}