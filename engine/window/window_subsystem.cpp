#include "window/window_subsystem.h"

#include <iostream>

#include "core/engine.h"
#include "window/window.h"
#include "rendering/render_subsystem.h"


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
	}

	void WindowSubsystem::Deinitialize()
	{
	}

	void WindowSubsystem::Update(double deltaTime)
	{
		Subsystem::Update(deltaTime);

		auto* renderSubsystem = mEngine->GetRenderSubsystem();

		if (mPrimaryWindow->GetIsResized())
		{
			Size newSize = mPrimaryWindow->GetSize();
			renderSubsystem->WindowResized(newSize);
		}
	}

	core::SubsystemType WindowSubsystem::GetType() const
	{
		return core::SubsystemType::Window;
	}

	Window* WindowSubsystem::GetPrimaryWindow() const
	{
		return mPrimaryWindow;
	}

	bool WindowSubsystem::ShouldPrimaryWindowClose() const
	{
		return mPrimaryWindow->ShouldClose();
	}

	Size WindowSubsystem::GetPrimaryWindowSize() const
	{
		return mPrimaryWindow->GetSize();
	}

	uint32_t WindowSubsystem::GetPrimaryWindowWidth() const
	{
		return mPrimaryWindow->GetWidth();
	}

	uint32_t WindowSubsystem::GetPrimaryWindowHeight() const
	{
		return mPrimaryWindow->GetHeight();
	}

	bool WindowSubsystem::GetPrimaryWindowFullscreen() const
	{
		return mPrimaryWindow->GetFullscreen();
	}

	void WindowSubsystem::SetPrimaryWindowFullscreen(bool fullscreen) const
	{
		mPrimaryWindow->SetFullscreen(fullscreen);
	}

	bool WindowSubsystem::GetPrimaryWindowBorderless() const
	{
		return mPrimaryWindow->GetBorderless();
	}

	void WindowSubsystem::SetPrimaryWindowBorderless(bool borderless) const
	{
		mPrimaryWindow->SetBorderless(borderless);
	}
}
