#pragma once

#include "Engine/Subsystem.hpp"
#include "Types/UUID.h"

#include <unordered_map>

class GLFWwindow;
class GLFWmonitor;

namespace puffin::Window
{
	class WindowSubsystem : public Core::Subsystem
	{
	public:

		WindowSubsystem() = default;
		~WindowSubsystem() override = default;

		void SetupCallbacks() override;

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

		// Create new window and return UUID handle to it
		UUID CreateNewWindow(const int& width, const int& height);

		// Retrieve window using UUID handle
		GLFWwindow* GetWindow(const UUID& uuid);

		void DestroyWindow(const UUID& uuid);

	private:

		GLFWmonitor* m_primaryMonitor = nullptr;
		GLFWwindow* m_primaryWindow = nullptr;

		std::unordered_map<UUID, GLFWwindow*> m_windows;

		
	};
}