#include "Engine.h"
#include "VulkanRenderer.h"
#include "EntitySystem.h"

#include <iostream>
#include <stdexcept>

int main()
{
	Engine engine;
	VulkanRenderer renderSystem;
	EntitySystem entitySystem;

	std::vector<uint32_t> entityIDs;

	try
	{
		engine.AddSystem(&renderSystem);
		engine.AddSystem(&entitySystem);

		for (int i = 0; i < 5; i++)
		{
			entityIDs.push_back(entitySystem.CreateEntity());
			entitySystem.GetEntity(entityIDs[i])->AttachComponent(renderSystem.AddComponent());
		}

		engine.MainLoop();
	}
	catch (const std::exception & e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}