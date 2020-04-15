#include "Engine.h"
#include "EntitySystem.h"
#include "VulkanRenderer.h"
#include "PhysicsSystem.h"
#include "TransformSystem.h"

#include <iostream>
#include <stdexcept>

int main()
{
	Engine engine;
	EntitySystem entitySystem;
	TransformSystem transformSystem;
	VulkanRenderer renderSystem;
	PhysicsSystem physicsSystem;

	std::vector<uint32_t> entityIDs;

	try
	{
		engine.AddSystem(&entitySystem);
		engine.AddSystem(&transformSystem);
		engine.AddSystem(&renderSystem);
		engine.AddSystem(&physicsSystem);

		for (int i = 0; i < 5; i++)
		{
			entityIDs.push_back(entitySystem.CreateEntity());
			entitySystem.GetEntity(entityIDs[i])->AttachComponent(transformSystem.AddComponent());
			entitySystem.GetEntity(entityIDs[i])->AttachComponent(renderSystem.AddComponent());
		}

		entitySystem.GetEntity(2)->AttachComponent(physicsSystem.AddComponent());
		entitySystem.GetEntity(5)->AttachComponent(physicsSystem.AddComponent());

		engine.MainLoop();
	}
	catch (const std::exception & e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}