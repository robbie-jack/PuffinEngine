#include "Engine.h"

#include <iostream>
#include <stdexcept>

int main()
{
	const auto engine = std::make_shared<Puffin::Core::Engine>();

	engine->Init();

	try
	{
		while(true)
		{
			if (!engine->Update())
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

	engine->Destroy();

	return EXIT_SUCCESS;
}