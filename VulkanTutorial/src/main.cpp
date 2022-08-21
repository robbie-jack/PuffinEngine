#include "Engine.h"

#include <iostream>
#include <stdexcept>

int main()
{
	Puffin::Core::Engine engine;

	engine.Init();

	try
	{
		//engine.MainLoop();

		while(true)
		{
			if (!engine.Update())
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

	engine.Destroy();

	return EXIT_SUCCESS;
}