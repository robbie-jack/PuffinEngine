#include <iostream>

#include "puffin/core/engine.h"
#include "puffin/physics/jolt/jolt_physics_system.h"
#include "puffin/rendering/vulkan/vk_render_system.h"
#include "puffin/scripting/angelscript/angelscript_system.h"

int main()
{
	const auto engine = std::make_shared<puffin::core::Engine>();

	engine->setup(R"(C:\Projects\PuffinProject\Puffin.pproject)");

	engine->registerSystem<puffin::rendering::VKRenderSystem>();
	engine->registerSystem<puffin::physics::JoltPhysicsSystem>();
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
