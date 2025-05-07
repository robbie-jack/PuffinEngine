#include "puffin/platform/raylib/window/windowsubsystemrl.h"

#include "raylib-cpp.hpp"

#include "puffin/platform/raylib/window/windowrl.h"

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
	}

	void WindowSubsystemRL::Deinitialize()
	{
		delete mPrimaryWindow;
		mPrimaryWindow = nullptr;
	}
}
