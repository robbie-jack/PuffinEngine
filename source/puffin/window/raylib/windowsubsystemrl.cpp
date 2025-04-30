#include "puffin/window/raylib/windowsubsystemrl.h"

#include "raylib.h"

namespace puffin::window
{
	WindowSubsystemRL::WindowSubsystemRL(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
		mName = "WindowSubsystemRL";
	}

	WindowSubsystemRL::~WindowSubsystemRL()
	{
		mEngine = nullptr;
	}

	void WindowSubsystemRL::Initialize(core::SubsystemManager* subsystemManager)
	{
		constexpr unsigned int flags = FLAG_WINDOW_RESIZABLE;

		mWindow = new raylib::Window(1920, 1080, "Puffin Engine", flags);
	}

	void WindowSubsystemRL::Deinitialize()
	{
		
	}

	raylib::Window* WindowSubsystemRL::GetWindow()
	{
		return mWindow;
	}
}
