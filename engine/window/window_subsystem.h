#pragma once

#include "core/subsystem.h"
#include "types/size.h"

namespace puffin
{
	namespace core
	{
		class Engine;
	}

	namespace window
	{
		class Window;

		class WindowSubsystem : public core::Subsystem
		{
		public:

			explicit WindowSubsystem(const std::shared_ptr<core::Engine>& engine);
			~WindowSubsystem() override;

			void Initialize(core::SubsystemManager* subsystemManager) override;
			void Deinitialize() override;

			void Update(double deltaTime) override;

			[[nodiscard]] core::SubsystemType GetType() const override;

			[[nodiscard]] Window* GetPrimaryWindow() const;

			[[nodiscard]] bool ShouldPrimaryWindowClose() const;
			[[nodiscard]] Size GetPrimaryWindowSize() const;
			[[nodiscard]] uint32_t GetPrimaryWindowWidth() const;
			[[nodiscard]] uint32_t GetPrimaryWindowHeight() const;

			[[nodiscard]] bool GetPrimaryWindowFullscreen() const;
			void SetPrimaryWindowFullscreen(bool fullscreen) const;

			[[nodiscard]] bool GetPrimaryWindowBorderless() const;
			void SetPrimaryWindowBorderless(bool borderless) const;

		protected:

			Window* mPrimaryWindow = nullptr;

		private:

			

		};
	}
}