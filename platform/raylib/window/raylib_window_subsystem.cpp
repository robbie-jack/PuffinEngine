#include "raylib/window/raylib_window_subsystem.h"

#include "raylib-cpp.hpp"

#include "raylib/window/raylib_window.h"

namespace puffin::window
{
	WindowSubsystemRL::WindowSubsystemRL(const std::shared_ptr<core::Engine>& engine) : WindowSubsystem(engine)
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

		mPrimaryWindow = new WindowRL(1920, 1080, "Puffin Engine", flags);
		mPrimaryWindow->SetMaximized(true);
	}

	void WindowSubsystemRL::Deinitialize()
	{
		delete mPrimaryWindow;
		mPrimaryWindow = nullptr;
	}
}
