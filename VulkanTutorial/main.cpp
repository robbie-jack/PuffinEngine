#include "Engine.h"
#include "VulkanRenderer.h"

#include <iostream>
#include <stdexcept>

int main()
{
	Engine engine;
	VulkanRenderer render_system;

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