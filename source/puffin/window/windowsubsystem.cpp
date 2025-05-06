#include "puffin/window/windowsubsystem.h"

#include <iostream>

#include "puffin/core/engine.h"


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

	core::SubsystemType WindowSubsystem::GetType() const
	{
		return core::SubsystemType::Window;
	}

	void WindowSubsystem::InitSettingsAndSignals()
	{

	}
}
