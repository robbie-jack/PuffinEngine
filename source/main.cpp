#include <iostream>

#include "puffin/core/engine.h"

#ifdef JOLT_PHYSICS_SUPPORT
#include "puffin/physics/jolt/jolt_physics_system.h"
#endif

#ifdef ONAGER2D_PHYSICS_SUPPORT
#include "puffin/physics/onager2d/onager_physics_system_2d.h"
#endif

#include "puffin/scripting/angelscript/angelscript_system.h"
#include "puffin/rendering/vulkan/render_system_vk.h"

int main()
{
	const auto engine = std::make_shared<puffin::core::Engine>();

	engine->setup(R"(C:\Projects\PuffinProject\Puffin.pproject)");

	engine->registerSystem<puffin::rendering::RenderSystemVK>();

#ifdef JOLT_PHYSICS_SUPPORT
	engine->registerSystem<puffin::physics::JoltPhysicsSystem>();
#endif

#ifdef ONAGER2D_PHYSICS_SUPPORT
    engine->registerSystem<puffin::physics::OnagerPhysicsSystem2D>();
#endif

	engine->registerSystem<puffin::scripting::AngelScriptSystem>();

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
