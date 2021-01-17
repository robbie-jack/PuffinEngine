#include "Engine.h"

#include <iostream>
#include <stdexcept>

int main()
{
	Puffin::Engine engine;

	try
	{
		engine.MainLoop();
	}
	catch (const std::exception & e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}