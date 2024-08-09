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
		m_engine->register_callback(core::ExecutionStage::StartupSubsystem, [&]() { startup(); }, "WindowSubsystem: startup", 40);
		m_engine->register_callback(core::ExecutionStage::ShutdownSubsystem, [&]() { shutdown(); }, "WindowSubsystem: shutdown", 150);
	}

	void WindowSubsystem::startup()
	{
		glfwInit();
		
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		mPrimaryMonitor = glfwGetPrimaryMonitor();

		// Create Primary Window
		mPrimaryWindow = glfwCreateWindow(1920, 1080, "Puffin Engine", nullptr, nullptr);
		if (mPrimaryWindow == nullptr)
		{
			std::cout << "Failed to create GLFW window" << std::endl;
			glfwTerminate();
		}

		glfwMakeContextCurrent(mPrimaryWindow);
	}

	void WindowSubsystem::shutdown()
	{
		glfwDestroyWindow(mPrimaryWindow);

		for (auto& [fst, snd] : mWindows)
		{
			glfwDestroyWindow(snd);
		}

		mWindows.clear();

		glfwTerminate();
	}

	bool WindowSubsystem::shouldPrimaryWindowClose() const
	{
		return glfwWindowShouldClose(mPrimaryWindow);
	}

	PuffinID WindowSubsystem::createNewWindow(const int& width, const int& height)
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

		mWindows.insert({ id, window });

		// Return id handle to window
		return id;
	}

	GLFWwindow* WindowSubsystem::getWindow(const PuffinID& uuid)
	{
		// Return window if there is oen with that handle
		if (mWindows.count(uuid) == 1)
		{
			return mWindows[uuid];
		}

		// Else return nullptr
		return nullptr;
	}

	void WindowSubsystem::destroyWindow(const PuffinID& uuid)
	{
		glfwDestroyWindow(mWindows[uuid]);
		mWindows.erase(uuid);
	}

	//==================================================
	// Private Methods
	//==================================================
}
