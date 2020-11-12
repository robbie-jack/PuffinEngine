#include "Engine.h"
//#include "EntitySystem.h"
//#include "TransformSystem.h"
//#include "VulkanRenderer.h"
//#include "ReactPhysicsSystem.h"

//#include "UIWindowSceneHierarchy.h"
//#include "UIWindowViewport.h"
//#include "UIWindowSettings.h"
//#include "UIWindowEntityProperties.h"
//#include "UIWindowPerformance.h"

#include <iostream>
#include <stdexcept>

//using namespace Puffin;
//using namespace Puffin::Rendering;
//using namespace Puffin::Physics;

int main()
{
	Puffin::Engine engine;
	/*EntitySystem entitySystem;
	TransformSystem transformSystem;
	ReactPhysicsSystem physicsSystem;
	VulkanRenderer renderSystem;

	Puffin::UI::UIManager uiManager;
	Puffin::Input::InputManager inputManager;*/

	try
	{
		//engine.AddSystem(&entitySystem);
		//engine.AddSystem(&physicsSystem);
		//engine.AddSystem(&transformSystem);
		//engine.AddSystem(&renderSystem);

		////uiManager.SetEngine(&engine);

		//renderSystem.SetUI(&uiManager);
		//renderSystem.SetInputManager(&inputManager);

		//transformSystem.SetPhysicsRenderVectors(physicsSystem.GetComponents(), renderSystem.GetComponents());

		//for (int i = 0; i < 5; i++)
		//{
		//	uint32_t id = entitySystem.CreateEntity();
		//	entitySystem.GetEntity(id)->AttachComponent(transformSystem.AddComponent());
		//	entitySystem.GetEntity(id)->AttachComponent(renderSystem.AddComponent());
		//}

		//entitySystem.GetEntity(3)->AttachComponent(physicsSystem.AddComponent());
		//entitySystem.GetEntity(5)->AttachComponent(physicsSystem.AddComponent());

		engine.MainLoop();
	}
	catch (const std::exception & e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}