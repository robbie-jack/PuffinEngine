#pragma once

#include "Core/Subsystem.h"
#include "Types/UUID.h"

#include <unordered_map>

class GLFWwindow;
class GLFWmonitor;

namespace puffin::window
{
	class WindowSubsystem : public core::Subsystem
	{
	public:

		WindowSubsystem() = default;
		~WindowSubsystem() override = default;

		void setupCallbacks() override;

		void init();
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