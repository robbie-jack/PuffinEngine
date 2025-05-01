#pragma once

#include <unordered_map>

#include "puffin/types/uuid.h"
#include "puffin/core/subsystem.h"

//class GLFWwindow;
//class GLFWmonitor;

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

			//GLFWwindow* GetPrimaryWindow() const
			//{
			//	return mPrimaryWindow;
			//}

			//[[nodiscard]] bool GetShouldPrimaryWindowClose() const;
			//GLFWmonitor* GetPrimaryMonitor() const;

			//// Create new window and return PuffinId handle to it
			//UUID CreateNewWindow(const int& width, const int& height);

			//// Retrieve window using PuffinId handle
			//GLFWwindow* GetWindow(const UUID& uuid);

			//void DestroyWindow(const UUID& uuid);

		private:

			/*GLFWmonitor* mPrimaryMonitor = nullptr;
			GLFWwindow* mPrimaryWindow = nullptr;

			std::unordered_map<UUID, GLFWwindow*> mWindows;*/


		};
	}
}