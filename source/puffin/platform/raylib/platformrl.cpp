#include <utility>

#include "puffin/core/engine.h"
#include "puffin/platform/raylib/platformrl.h"
#include "puffin/platform/raylib/window/windowsubsystemrl.h"
#include "puffin/platform/raylib/input/inputsubsystemrl.h"
#include "puffin/platform/raylib/rendering/2d/rendersubsystemrl2d.h"

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
