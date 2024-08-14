#include <iostream>

#include "puffin/core/engine.h"
#include "argparse/argparse.hpp"

#ifdef PFN_BOX2D_PHYSICS
#include "puffin/physics/box2d/box2d_physics_system.h"
#endif

#ifdef PFN_JOLT_PHYSICS
#include "puffin/physics/jolt/jolt_physics_system.h"
#endif

#ifdef PFN_ONAGER2D_PHYSICS
#include "puffin/physics/onager2d/onager_physics_system_2d.h"
#endif

#include "puffin/scripting/angelscript/angelscript_system.h"
#include "puffin/rendering/vulkan/render_system_vk.h"

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
