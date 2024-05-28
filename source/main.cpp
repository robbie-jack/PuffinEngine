#include <iostream>

#include "puffin/core/engine.h"

#ifdef PFN_JOLT_PHYSICS
#include "puffin/physics/jolt/jolt_physics_system.h"
#endif

#ifdef PFN_ONAGER2D_PHYSICS
#include "puffin/physics/onager2d/onager_physics_system_2d.h"
#endif

#include "puffin/scripting/angelscript/angelscript_system.h"
#include "puffin/rendering/vulkan/render_system_vk.h"

int main()
{
	const auto engine = std::make_shared<puffin::core::Engine>();

	engine->setup(R"(C:\Projects\PuffinProject\Puffin.pproject)");

	engine->register_system<puffin::rendering::RenderSystemVK>();

#ifdef PFN_JOLT_PHYSICS
	engine->register_system<puffin::physics::JoltPhysicsSystem>();
#endif

#ifdef PFN_ONAGER2D_PHYSICS
    engine->register_system<puffin::physics::OnagerPhysicsSystem2D>();
#endif

	engine->register_system<puffin::scripting::AngelScriptSystem>();

	engine->startup();

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

	engine->shutdown();

	return EXIT_SUCCESS;
}
