#include "raylib/platform_rl.h"

#include <utility>

#include "puffin/core/engine.h"
#include "raylib/window/window_subsystem_rl.h"
#include "raylib/input/input_subsystem_rl.h"
#include "raylib/rendering/2d/render_subsystem_rl_2d.h"

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
