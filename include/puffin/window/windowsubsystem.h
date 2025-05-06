#pragma once

#include "puffin/core/subsystem.h"
#include "puffin/types/size.h"

namespace puffin
{
	namespace core
	{
		class Engine;
	}

	namespace window
	{
		struct WindowSettings
		{
			Size screenSize;
		};

		class WindowSubsystem : public core::Subsystem
		{
		public:

			explicit WindowSubsystem(const std::shared_ptr<core::Engine>& engine);
			~WindowSubsystem() override;

			void Initialize(core::SubsystemManager* subsystemManager) override;
			void Deinitialize() override;

			[[nodiscard]] core::SubsystemType GetType() const override;

			[[nodiscard]] virtual bool ShouldPrimaryWindowClose() const = 0;
			[[nodiscard]] virtual Size GetPrimaryWindowSize() const = 0;
			[[nodiscard]] virtual uint32_t GetPrimaryWindowWidth() const = 0;
			[[nodiscard]] virtual uint32_t GetPrimaryWindowHeight() const = 0;

		private:

			void InitSettingsAndSignals();

		};
	}
}