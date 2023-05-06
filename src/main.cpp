#include "Core/Engine.h"

#include <iostream>

int main()
{
	const auto engine = std::make_shared<puffin::core::Engine>();

	engine->init();

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

	engine->destroy();

	return EXIT_SUCCESS;
}