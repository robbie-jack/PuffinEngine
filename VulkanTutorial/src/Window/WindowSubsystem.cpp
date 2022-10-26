#include "Window/WindowSubsystem.hpp"

#include <glfw/glfw3.h>

#include <iostream>


namespace Puffin::Window
{
	//==================================================
	// Public Methods
	//==================================================

	void WindowSubsystem::Init()
	{
		glfwInit();
		
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		m_primaryMonitor = glfwGetPrimaryMonitor();

		// Create Primary Window
		m_primaryWindow = glfwCreateWindow(1280, 720, "Puffin Engine", nullptr, nullptr);
		if (m_primaryWindow == nullptr)
		{
			std::cout << "Failed to create GLFW window" << std::endl;
			glfwTerminate();
		}

		glfwMakeContextCurrent(m_primaryWindow);
	}

	void WindowSubsystem::Update()
	{
		
	}

	void WindowSubsystem::Destroy()
	{
		glfwDestroyWindow(m_primaryWindow);

		for (auto& [fst, snd] : m_windows)
		{
			glfwDestroyWindow(snd);
		}

		m_windows.clear();

		glfwTerminate();
	}

	bool WindowSubsystem::ShouldPrimaryWindowClose() const
	{
		return glfwWindowShouldClose(m_primaryWindow);
	}

	UUID WindowSubsystem::CreateNewWindow(const int& width, const int& height)
	{
		// Create new window and store it in windows map
		GLFWwindow* window = glfwCreateWindow(width, height, "Puffin Engine", nullptr, nullptr);
		if (window == nullptr)
		{
			std::cout << "Failed to create GLFW window" << std::endl;
			glfwTerminate();
			return 0;
		}

		UUID id;

		m_windows.insert({ id, window });

		// Return id handle to window
		return id;
	}

	GLFWwindow* WindowSubsystem::GetWindow(const UUID& uuid)
	{
		// Return window if there is oen with that handle
		if (m_windows.count(uuid) == 1)
		{
			return m_windows[uuid];
		}

		// Else return nullptr
		return nullptr;
	}

	void WindowSubsystem::DestroyWindow(const UUID& uuid)
	{
		glfwDestroyWindow(m_windows[uuid]);
		m_windows.erase(uuid);
	}

	//==================================================
	// Private Methods
	//==================================================
}
