#include "raylib/window/window_rl.h"

#include "puffin/types/size.h"
#include "raylib-cpp.hpp"

namespace puffin::window
{
	WindowRL::WindowRL(int32_t width, int32_t height, const std::string& name, uint32_t flags)
		: Window(width, height, name, flags)
	{
		mWindow = new raylib::Window(1920, 1080, "Puffin Engine", flags);
	}

	WindowRL::~WindowRL()
	{
		delete mWindow;
		mWindow = nullptr;
	}

	bool WindowRL::ShouldClose() const
	{
		return mWindow->ShouldClose();
	}

	Size WindowRL::GetSize() const
	{
		return { static_cast<uint32_t>(mWindow->GetWidth()),
			static_cast<uint32_t>(mWindow->GetHeight()) };
	}

	uint32_t WindowRL::GetWidth() const
	{
		return static_cast<uint32_t>(mWindow->GetWidth());
	}

	uint32_t WindowRL::GetHeight() const
	{
		return static_cast<uint32_t>(mWindow->GetHeight());
	}

	bool WindowRL::GetIsResized() const
	{
		return mWindow->IsResized();
	}

	bool WindowRL::GetFullscreen() const
	{
		return mWindow->IsFullscreen();
	}

	void WindowRL::SetFullscreen(bool fullscreen)
	{
		mWindow->SetFullscreen(fullscreen);
	}

	bool WindowRL::GetBorderless() const
	{
		return mWindow->IsState(FLAG_BORDERLESS_WINDOWED_MODE);
	}

	void WindowRL::SetBorderless(bool borderless)
	{
		if (GetBorderless() != borderless)
			mWindow->ToggleBorderless();
	}

	bool WindowRL::GetMaximized() const
	{
		return mWindow->IsMaximized();
	}

	void WindowRL::SetMaximized(bool maximized)
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
