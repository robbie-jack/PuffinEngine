#include "puffin/window/windowsubsystem.h"

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
		mName = "WindowSubsystem";
	}

	WindowSubsystem::~WindowSubsystem()
	{
		mEngine = nullptr;
	}

	void WindowSubsystem::Initialize(core::SubsystemManager* subsystemManager)
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

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

	void WindowSubsystem::Deinitialize()
	{
		glfwDestroyWindow(mPrimaryWindow);

		for (auto& [fst, snd] : mWindows)
		{
			glfwDestroyWindow(snd);
		}

		mWindows.clear();

		glfwTerminate();
	}

	bool WindowSubsystem::GetShouldPrimaryWindowClose() const
	{
		return glfwWindowShouldClose(mPrimaryWindow);
	}

	GLFWmonitor* WindowSubsystem::GetPrimaryMonitor() const
	{
		return mPrimaryMonitor;
	}

	UUID WindowSubsystem::CreateNewWindow(const int& width, const int& height)
	{
		// Create new window and store it in windows map
		GLFWwindow* window = glfwCreateWindow(width, height, "Puffin Engine", nullptr, nullptr);
		if (window == nullptr)
		{
			std::cout << "Failed to create GLFW window" << std::endl;
			glfwTerminate();
			return {};
		}

		UUID id;

		mWindows.insert({ id, window });

		// Return id handle to window
		return id;
	}

	GLFWwindow* WindowSubsystem::GetWindow(const UUID& uuid)
	{
		// Return window if there is oen with that handle
		if (mWindows.count(uuid) == 1)
		{
			return mWindows[uuid];
		}

		// Else return nullptr
		return nullptr;
	}

	void WindowSubsystem::DestroyWindow(const UUID& uuid)
	{
		glfwDestroyWindow(mWindows[uuid]);
		mWindows.erase(uuid);
	}
}
