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

	WindowSubsystem::WindowSubsystem(const std::shared_ptr<core::Engine>& engine) : EngineSubsystem(engine)
	{
	}

	WindowSubsystem::~WindowSubsystem()
	{
		m_engine = nullptr;
	}

	void WindowSubsystem::Update(double deltaTime)
	{
		auto* renderSubsystem = m_engine->GetRenderSubsystem();

		if (m_primaryWindow->GetIsResized())
		{
			Size newSize = m_primaryWindow->GetSize();
			renderSubsystem->WindowResized(newSize);
		}
	}

	std::string_view WindowSubsystem::GetName() const
	{
		return reflection::GetTypeString<WindowSubsystem>();
	}

	Window* WindowSubsystem::GetPrimaryWindow() const
	{
		return m_primaryWindow;
	}

	bool WindowSubsystem::ShouldPrimaryWindowClose() const
	{
		return m_primaryWindow->ShouldClose();
	}

	Size WindowSubsystem::GetPrimaryWindowSize() const
	{
		return m_primaryWindow->GetSize();
	}

	uint32_t WindowSubsystem::GetPrimaryWindowWidth() const
	{
		return m_primaryWindow->GetWidth();
	}

	uint32_t WindowSubsystem::GetPrimaryWindowHeight() const
	{
		return m_primaryWindow->GetHeight();
	}

	bool WindowSubsystem::GetPrimaryWindowFullscreen() const
	{
		return m_primaryWindow->GetFullscreen();
	}

	void WindowSubsystem::SetPrimaryWindowFullscreen(bool fullscreen) const
	{
		m_primaryWindow->SetFullscreen(fullscreen);
	}

	bool WindowSubsystem::GetPrimaryWindowBorderless() const
	{
		return m_primaryWindow->GetBorderless();
	}

	void WindowSubsystem::SetPrimaryWindowBorderless(bool borderless) const
	{
		m_primaryWindow->SetBorderless(borderless);
	}
}
