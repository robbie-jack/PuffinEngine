#include "Engine.h"
#include "EntitySystem.h"
#include "TransformSystem.h"
#include "VulkanRenderer.h"
#include "ReactPhysicsSystem.h"

#include "UIWindowEntities.h"
#include "UIWindowViewport.h"
#include "UIWindowSettings.h"
#include "UIWindowEntityProperties.h"
#include "UIWindowPerformance.h"

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

	Puffin::UI::UIManager uiManager;
	Puffin::Input::InputManager inputManager;

	Puffin::UI::UIWindowEntities windowEntities;
	Puffin::UI::UIWindowViewport windowViewport;
	Puffin::UI::UIWindowSettings windowSettings;
	Puffin::UI::UIWindowEntityProperties windowEntityProperties;
	Puffin::UI::UIWindowPerformance windowPerformance;

	std::vector<uint32_t> entityIDs;

	try
	{
		engine.AddSystem(&entitySystem);
		engine.AddSystem(&physicsSystem);
		engine.AddSystem(&transformSystem);
		engine.AddSystem(&renderSystem);

		windowPerformance.Show();

		uiManager.AddWindow(&windowEntities);
		uiManager.AddWindow(&windowViewport);
		uiManager.AddWindow(&windowSettings);
		uiManager.AddWindow(&windowEntityProperties);
		uiManager.AddWindow(&windowPerformance);
		uiManager.SetEngine(&engine);

		windowEntities.SetEntitySystem(&entitySystem);
		windowEntities.SetWindowProperties(&windowEntityProperties);

		renderSystem.SetUI(&uiManager);
		renderSystem.SetUIWindowViewport(&windowViewport);
		renderSystem.SetUIWindowSettings(&windowSettings);
		renderSystem.SetInputManager(&inputManager);

		transformSystem.SetPhysicsRenderVectors(physicsSystem.GetComponents(), renderSystem.GetComponents());

		for (int i = 0; i < 5; i++)
		{
			uint32_t id = entitySystem.CreateEntity();
			entitySystem.GetEntity(id)->AttachComponent(transformSystem.AddComponent());
			entitySystem.GetEntity(id)->AttachComponent(renderSystem.AddComponent());
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