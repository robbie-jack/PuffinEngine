#pragma once

#include "puffin/core/subsystem.h"
#include "puffin/types/vector2.h"

namespace puffin
{
	namespace core
	{
		class Engine;
	}

	namespace window
	{
		class WindowSubsystem : public core::Subsystem
		{
		public:

			explicit WindowSubsystem(const std::shared_ptr<core::Engine>& engine);
			~WindowSubsystem() override;

			void Initialize(core::SubsystemManager* subsystemManager) override;
			void Deinitialize() override;

			[[nodiscard]] core::SubsystemType GetType() const override;

			[[nodiscard]] virtual bool ShouldPrimaryWindowClose() const = 0;

			[[nodiscard]] virtual Vector2i GetPrimaryWindowSize() const = 0;
			[[nodiscard]] virtual int GetPrimaryWindowWidth() const = 0;
			[[nodiscard]] virtual int GetPrimaryWindowHeight() const = 0;

		private:



		};
	}
}