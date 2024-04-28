#pragma once

#include <unordered_map>

#include "puffin/types/uuid.h"
#include "puffin/core/system.h"

class GLFWwindow;
class GLFWmonitor;

namespace puffin
{
	namespace core
	{
		class Engine;
	}

	namespace window
	{
		class WindowSubsystem : public core::System
		{
		public:

			WindowSubsystem(const std::shared_ptr<core::Engine>& engine);
			~WindowSubsystem() override { mEngine = nullptr; }

			void startup();
			void shutdown();

			GLFWwindow* primaryWindow() const
			{
				return mPrimaryWindow;
			}

			[[nodiscard]] bool shouldPrimaryWindowClose() const;

			GLFWmonitor* primaryMonitor() const
			{
				return mPrimaryMonitor;
			}

			// Create new window and return PuffinId handle to it
			PuffinID createNewWindow(const int& width, const int& height);

			// Retrieve window using PuffinId handle
			GLFWwindow* getWindow(const PuffinID& uuid);

			void destroyWindow(const PuffinID& uuid);

		private:

			GLFWmonitor* mPrimaryMonitor = nullptr;
			GLFWwindow* mPrimaryWindow = nullptr;

			std::unordered_map<PuffinID, GLFWwindow*> mWindows;


		};
	}
}