#include <iostream>

#include "editor.h"
#include "argparse/argparse.hpp"

#ifdef PFN_BOX2D_PHYSICS
#include "physics/box2d/box2d_physics_subsystem.h"
#endif

#ifdef PFN_JOLT_PHYSICS
#include "physics/jolt/jolt_physics_subsystem.h"
#endif

#ifdef PFN_ONAGER2D_PHYSICS
#include "physics/onager2d/onager2d_physics_subsystem.h"
#endif

//#include "puffin/rendering/vulkan/rendersubsystemvk.h"

#include "core/engine_helpers.h"
#include "raylib/raylib_platform.h"

int main(int argc, char* argv[])
{
    argparse::ArgumentParser parser("PuffinEditor");

    puffin::AddDefaultEngineArguments(parser);

    try
    {
        parser.parse_args(argc, argv);
    }
    catch (const std::exception& err)
    {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        std::exit(1);
    }

	auto editor = std::make_shared<puffin::editor::Editor>();
	const auto engine = editor->GetEngine();

	engine->RegisterPlatform<puffin::core::PlatformRL>();

	puffin::core::RegisterComponentTypes2D();
	puffin::core::RegisterNodeTypes2D();

	editor->Setup();

#ifdef PFN_BOX2D_PHYSICS
	engine->RegisterSubsystem<puffin::physics::Box2DPhysicsSubsystem>();
#endif

#ifdef PFN_JOLT_PHYSICS
	engine->RegisterSubsystem<puffin::physics::JoltPhysicsSubsystem>();
#endif

	editor->Initialize(parser);

	try
	{
		while(true)
		{
			if (!editor->Update())
			{
				break;
			}
		}
	}
	catch (const std::exception & e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	editor->Deinitialize();

	editor = nullptr;

	return EXIT_SUCCESS;
}
