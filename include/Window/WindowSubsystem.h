#pragma once

#include "Engine\Subsystem.h"
#include "Types/UUID.h"

#include <unordered_map>

class GLFWwindow;
class GLFWmonitor;

namespace puffin::Window
{
	class WindowSubsystem : public core::Subsystem
	{
	public:

		WindowSubsystem() = default;
		~WindowSubsystem() override = default;

		void setupCallbacks() override;

		void Init();
		void Update();
		void Cleanup();

		GLFWwindow* GetPrimaryWindow() const
		{
			return m_primaryWindow;
		}

		bool ShouldPrimaryWindowClose() const;

		GLFWmonitor* GetPrimaryMonitor() const
		{
			return m_primaryMonitor;
		}

		// Create new window and return PuffinId handle to it
		PuffinId CreateNewWindow(const int& width, const int& height);

		// Retrieve window using PuffinId handle
		GLFWwindow* GetWindow(const PuffinId& uuid);

		void DestroyWindow(const PuffinId& uuid);

	private:

		GLFWmonitor* m_primaryMonitor = nullptr;
		GLFWwindow* m_primaryWindow = nullptr;

		std::unordered_map<PuffinId, GLFWwindow*> m_windows;

		
	};
}