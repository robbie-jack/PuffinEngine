#include "VulkanRenderer.h"

#include <iostream>
#include <stdexcept>

int main()
{
	VulkanRenderer renderer;

	try
	{
		renderer.Run();
	}
	catch (const std::exception & e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}