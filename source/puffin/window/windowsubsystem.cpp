#include "puffin/window/window_subsystem.h"

#include <iostream>

#include "GLFW/glfw3.h"

#include "puffin/core/engine.h"


namespace puffin::window
{
	//==================================================
	// Public Methods
	//==================================================

	WindowSubsystem::WindowSubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
		m_name = "WindowSubsystem";
	}

	WindowSubsystem::~WindowSubsystem()
	{
		m_engine = nullptr;
	}

	void WindowSubsystem::initialize(core::SubsystemManager* subsystem_manager)
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		m_primary_monitor = glfwGetPrimaryMonitor();

		// Create Primary Window
		m_primary_window = glfwCreateWindow(1920, 1080, "Puffin Engine", nullptr, nullptr);
		if (m_primary_window == nullptr)
		{
			std::cout << "Failed to create GLFW window" << std::endl;
			glfwTerminate();
		}

		glfwMakeContextCurrent(m_primary_window);
	}

	void WindowSubsystem::deinitialize()
	{
		glfwDestroyWindow(m_primary_window);

		for (auto& [fst, snd] : m_windows)
		{
			glfwDestroyWindow(snd);
		}

		m_windows.clear();

		glfwTerminate();
	}

	bool WindowSubsystem::should_primary_window_close() const
	{
		return glfwWindowShouldClose(m_primary_window);
	}

	GLFWmonitor* WindowSubsystem::primary_monitor() const
	{
		return m_primary_monitor;
	}

	PuffinID WindowSubsystem::create_new_window(const int& width, const int& height)
	{
		// Create new window and store it in windows map
		GLFWwindow* window = glfwCreateWindow(width, height, "Puffin Engine", nullptr, nullptr);
		if (window == nullptr)
		{
			std::cout << "Failed to create GLFW window" << std::endl;
			glfwTerminate();
			return {};
		}

		PuffinID id;

		m_windows.insert({ id, window });

		// Return id handle to window
		return id;
	}

	GLFWwindow* WindowSubsystem::get_window(const PuffinID& uuid)
	{
		// Return window if there is oen with that handle
		if (m_windows.count(uuid) == 1)
		{
			return m_windows[uuid];
		}

		// Else return nullptr
		return nullptr;
	}

	void WindowSubsystem::destroy_window(const PuffinID& uuid)
	{
		glfwDestroyWindow(m_windows[uuid]);
		m_windows.erase(uuid);
	}

	//==================================================
	// Private Methods
	//==================================================
}
