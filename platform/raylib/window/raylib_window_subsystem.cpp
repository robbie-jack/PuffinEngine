#include "raylib/window/raylib_window_subsystem.h"

#include "raylib-cpp.hpp"

#include "raylib/window/raylib_window.h"

namespace puffin::window
{
	RaylibWindowSubsystem::RaylibWindowSubsystem(const std::shared_ptr<core::Engine>& engine) : WindowSubsystem(engine)
	{
	}

	RaylibWindowSubsystem::~RaylibWindowSubsystem()
	{
		m_engine = nullptr;
	}

	void RaylibWindowSubsystem::Initialize()
	{
		constexpr unsigned int flags = FLAG_WINDOW_RESIZABLE;

		m_primaryWindow = new RaylibWindow(1920, 1080, "Puffin Engine", flags);
		m_primaryWindow->SetMaximized(true);
	}

	void RaylibWindowSubsystem::Deinitialize()
	{
		delete m_primaryWindow;
		m_primaryWindow = nullptr;
	}

	std::string_view RaylibWindowSubsystem::GetName() const
	{
		return reflection::GetTypeString<window::RaylibWindowSubsystem>();
	}
}
