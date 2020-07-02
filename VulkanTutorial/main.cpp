#include "Engine.h"
#include "EntitySystem.h"
#include "TransformSystem.h"
#include "VulkanRenderer.h"
#include "ReactPhysicsSystem.h"

#include <iostream>
#include <stdexcept>

using namespace Puffin;
using namespace Puffin::Rendering;
using namespace Puffin::Physics;

int main()
{
	Engine engine;
	EntitySystem entitySystem;
	TransformSystem transformSystem;
	ReactPhysicsSystem physicsSystem;
	VulkanRenderer renderSystem;

	Puffin::UI::UserInterface UI;
	Puffin::Input::InputManager inputManager;

	std::vector<uint32_t> entityIDs;

	try
	{
		engine.AddSystem(&entitySystem);
		engine.AddSystem(&physicsSystem);
		engine.AddSystem(&transformSystem);
		engine.AddSystem(&renderSystem);

		renderSystem.SetUI(&UI);
		renderSystem.SetInputManager(&inputManager);

		transformSystem.SetPhysicsRenderVectors(physicsSystem.GetComponents(), renderSystem.GetComponents());

		for (int i = 0; i < 5; i++)
		{
			entityIDs.push_back(entitySystem.CreateEntity());
			entitySystem.GetEntity(entityIDs[i])->AttachComponent(transformSystem.AddComponent());
			entitySystem.GetEntity(entityIDs[i])->AttachComponent(renderSystem.AddComponent());
		}

		entitySystem.GetEntity(3)->AttachComponent(physicsSystem.AddComponent());
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