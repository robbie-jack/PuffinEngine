#include "raylib/window/raylib_window_subsystem.h"

#include "raylib-cpp.hpp"

#include "raylib/window/raylib_window.h"

namespace puffin::window
{
	RaylibWindowSubsystem::RaylibWindowSubsystem(const std::shared_ptr<core::Engine>& engine) : WindowSubsystem(engine)
	{
		mName = "RaylibWindowSubsystem";
	}

	RaylibWindowSubsystem::~RaylibWindowSubsystem()
	{
		mEngine = nullptr;
	}

	void RaylibWindowSubsystem::Initialize(core::SubsystemManager* subsystemManager)
	{
		constexpr unsigned int flags = FLAG_WINDOW_RESIZABLE;

		mPrimaryWindow = new RaylibWindow(1920, 1080, "Puffin Engine", flags);
		mPrimaryWindow->SetMaximized(true);
	}

	void RaylibWindowSubsystem::Deinitialize()
	{
		delete mPrimaryWindow;
		mPrimaryWindow = nullptr;
	}
}
