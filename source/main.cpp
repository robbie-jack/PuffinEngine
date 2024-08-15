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

    puffin::add_default_engine_arguments(parser);

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

	engine->setup();

	engine->register_subsystem<puffin::rendering::RenderSubystemVK>();

#ifdef PFN_BOX2D_PHYSICS
	engine->register_subsystem<puffin::physics::Box2DPhysicsSystem>();
#endif

#ifdef PFN_JOLT_PHYSICS
	engine->register_subsystem<puffin::physics::JoltPhysicsSubsystem>();
#endif

	engine->register_subsystem<puffin::scripting::AngelScriptSubsystem>();
	engine->register_subsystem<puffin::scripting::AngelScriptGameplaySubsystem>();

	engine->initialize(parser);

	try
	{
		while(true)
		{
			if (!engine->update())
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

	engine->deinitialize();

	return EXIT_SUCCESS;
}
