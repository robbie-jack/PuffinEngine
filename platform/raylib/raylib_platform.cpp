#include "raylib/raylib_platform.h"

#include <utility>

#include "core/engine.h"
#include "raylib/window/raylib_window_subsystem.h"
#include "raylib/input/raylib_input_subsystem.h"
#include "raylib/rendering/2d/raylib_2d_render_subsystem.h"

#include "raylib-cpp.hpp"

namespace puffin::core
{
	double GetTime()
	{
		return ::GetTime();
	}

	PlatformRL::PlatformRL(std::shared_ptr<Engine> engine)
		: Platform(std::move(engine))
	{
		mName = "Raylib";
	}

	void PlatformRL::RegisterTypes()
	{
		mEngine->RegisterSubsystem<window::WindowSubsystemRL>();
		mEngine->RegisterSubsystem<input::InputSubsystemRL>();
		mEngine->RegisterSubsystem<rendering::RenderSubsystemRL2D>();
	}
}
