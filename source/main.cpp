#include <iostream>

#include "puffin/core/engine.h"
#include "argparse/argparse.hpp"

#ifdef PFN_BOX2D_PHYSICS
#include "puffin/physics/box2d/box2dphysicssystem.h"
#endif

#ifdef PFN_JOLT_PHYSICS
#include "puffin/physics/jolt/joltphysicssubsystem.h"
#endif

#ifdef PFN_ONAGER2D_PHYSICS
#include "puffin/physics/onager2d/onager2dphysicssystem.h"
#endif

#include "puffin/scripting/angelscript/angelscriptsubsystem.h"
#include "puffin/rendering/vulkan/rendersubsystemvk.h"

int main(int argc, char* argv[])
{
    argparse::ArgumentParser parser("puffin app");

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

	const auto engine = std::make_shared<puffin::core::Engine>();

	engine->Setup();

	engine->RegisterSubsystem<puffin::rendering::RenderSubsystemVK>();

#ifdef PFN_BOX2D_PHYSICS
	engine->register_subsystem<puffin::physics::Box2DPhysicsSystem>();
#endif

#ifdef PFN_JOLT_PHYSICS
	engine->RegisterSubsystem<puffin::physics::JoltPhysicsSubsystem>();
#endif

	engine->RegisterSubsystem<puffin::scripting::AngelScriptSubsystem>();
	engine->RegisterSubsystem<puffin::scripting::AngelScriptGameplaySubsystem>();

	engine->Initialize(parser);

	try
	{
		while(true)
		{
			if (!engine->Update())
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

	engine->Deinitialize();

	return EXIT_SUCCESS;
}
