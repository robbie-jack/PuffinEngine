#include <utility>

#include "puffin/core/engine.h"
#include "puffin/platform/platformrl.h"
#include "puffin/window/raylib/windowsubsystemrl.h"
#include "puffin/rendering/raylib/2d/rendersubsystemrl2d.h"

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
		mEngine->RegisterSubsystem<puffin::window::WindowSubsystemRL>();
		mEngine->RegisterSubsystem<puffin::rendering::RenderSubsystemRL2D>();
	}
}
