#include "VulkanRenderer.h"
#include "Engine.h"
#include "RenderSystem.h"

#include <iostream>
#include <stdexcept>

int main()
{
	Engine engine;
	RenderSystem render_system;

	try
	{
		engine.AddSystem(&render_system);
		engine.MainLoop();
	}
	catch (const std::exception & e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}