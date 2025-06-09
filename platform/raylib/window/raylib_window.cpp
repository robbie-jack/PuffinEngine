#include "raylib/window/raylib_window.h"

#include "types/size.h"
#include "raylib-cpp.hpp"

namespace puffin::window
{
	RaylibWindow::RaylibWindow(int32_t width, int32_t height, const std::string& name, uint32_t flags)
		: Window(width, height, name, flags)
	{
		mWindow = new raylib::Window(1920, 1080, "Puffin Engine", flags);
	}

	RaylibWindow::~RaylibWindow()
	{
		delete mWindow;
		mWindow = nullptr;
	}

	bool RaylibWindow::ShouldClose() const
	{
		return mWindow->ShouldClose();
	}

	Size RaylibWindow::GetSize() const
	{
		return { static_cast<uint32_t>(mWindow->GetWidth()),
			static_cast<uint32_t>(mWindow->GetHeight()) };
	}

	uint32_t RaylibWindow::GetWidth() const
	{
		return static_cast<uint32_t>(mWindow->GetWidth());
	}

	uint32_t RaylibWindow::GetHeight() const
	{
		return static_cast<uint32_t>(mWindow->GetHeight());
	}

	bool RaylibWindow::GetIsResized() const
	{
		return mWindow->IsResized();
	}

	bool RaylibWindow::GetFullscreen() const
	{
		return mWindow->IsFullscreen();
	}

	void RaylibWindow::SetFullscreen(bool fullscreen)
	{
		mWindow->SetFullscreen(fullscreen);
	}

	bool RaylibWindow::GetBorderless() const
	{
		return mWindow->IsState(FLAG_BORDERLESS_WINDOWED_MODE);
	}

	void RaylibWindow::SetBorderless(bool borderless)
	{
		if (GetBorderless() != borderless)
			mWindow->ToggleBorderless();
	}

	bool RaylibWindow::GetMaximized() const
	{
		return mWindow->IsMaximized();
	}

	void RaylibWindow::SetMaximized(bool maximized)
	{
		if (maximized)
		{
			mWindow->Maximize();
		}
		else
		{
			mWindow->Restore();
		}
	}
}
