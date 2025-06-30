#include "raylib/raylib_platform.h"

#include <utility>

#include "core/engine.h"
#include "subsystem/subsystem_reflection.h"
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

	RaylibPlatform::RaylibPlatform(std::shared_ptr<Engine> engine)
		: Platform(std::move(engine))
	{
		mName = "Raylib";
	}

	void RaylibPlatform::PreInitialize()
	{
		reflection::RegisterType<window::RaylibWindowSubsystem>();
		reflection::RegisterType<input::RaylibInputSubsystem>();
		reflection::RegisterType<rendering::Raylib2DRenderSubsystem>();
	}

	void RaylibPlatform::Initialize()
	{

	}

	void RaylibPlatform::PostInitialize()
	{

	}
}
