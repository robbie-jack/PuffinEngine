#pragma once

#include "glfw/glfw3.h"

#include "Engine/Subsystem.hpp"
#include "Types/UUID.h"

#include <unordered_map>

namespace Puffin::Window
{
	class WindowSubsystem : public Core::Subsystem
	{
	public:

		WindowSubsystem() = default;
		~WindowSubsystem() override = default;

		void Init() override;
		void Update() override;
		void Destroy() override;

		GLFWwindow* GetPrimaryWindow() const
		{
			return m_primaryWindow;
		}

		bool ShouldPrimaryWindowClose() const
		{
			return glfwWindowShouldClose(m_primaryWindow);
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