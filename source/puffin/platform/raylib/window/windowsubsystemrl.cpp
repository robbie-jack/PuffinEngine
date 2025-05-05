#include "puffin/platform/raylib/window/windowsubsystemrl.h"

#include "raylib.h"

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

		mPrimaryWindow = new raylib::Window(1920, 1080, "Puffin Engine", flags);
	}

	void WindowSubsystemRL::Deinitialize()
	{
		
	}

	bool WindowSubsystemRL::ShouldPrimaryWindowClose() const
	{
		if (mPrimaryWindow)
			return mPrimaryWindow->ShouldClose();

		return true;
	}

	Vector2i WindowSubsystemRL::GetPrimaryWindowSize() const
	{
		return { mPrimaryWindow->GetWidth(), mPrimaryWindow->GetHeight() };
	}

	int WindowSubsystemRL::GetPrimaryWindowWidth() const
	{
		return mPrimaryWindow->GetWidth();
	}

	int WindowSubsystemRL::GetPrimaryWindowHeight() const
	{
		return mPrimaryWindow->GetHeight();
	}

	raylib::Window* WindowSubsystemRL::GetPrimaryWindow()
	{
		return mPrimaryWindow;
	}
}
