#include "JinxScriptingSystem.h"
#include <fstream>
#include <iostream>

namespace Puffin
{
	namespace Scripting
	{
		// Public Functions
		void JinxScriptingSystem::Start()
		{
			// Create Jinx runtime object
			runtime = Jinx::CreateRuntime();

			InitLibraries();

			// Compile Bytecode for script components
			for (ECS::Entity entity : entityMap["Script"])
			{
				JinxScriptComponent& comp = world->GetComponent<JinxScriptComponent>(entity);
				InitComponent(comp);
			}
		}

		bool JinxScriptingSystem::Update(float dt)
		{
			// Update Core Library Properties
			libraryComponents["Puffin-Core"].Script->SetVariable("inDeltaTime", dt);
			libraryComponents["Puffin-Core"].Script->Execute();

			// Execute Script Components
			for (ECS::Entity entity : entityMap["Script"])
			{
				JinxScriptComponent& comp = world->GetComponent<JinxScriptComponent>(entity);

				if (comp.Script)
				{
					comp.Script->Execute();
				}
			}

			return true;
		}

		void JinxScriptingSystem::Stop()
		{
			runtime = nullptr;

			libraryComponents.clear();

			for (ECS::Entity entity : entityMap["Script"])
			{
				JinxScriptComponent& comp = world->GetComponent<JinxScriptComponent>(entity);

				comp.Bytecode = nullptr;
				comp.Script = nullptr;
			}
		}

		// Private Functions

		// Load and compile library scripts
		void JinxScriptingSystem::InitLibraries()
		{
			// Compile Core Puffin Library
			JinxScriptComponent puffinCoreComp;
			puffinCoreComp.Name = "Puffin-Core";
			puffinCoreComp.Dir = "src/Scripting/puffin-core.jnx";
			InitComponent(puffinCoreComp);

			libraryComponents["Puffin-Core"] = puffinCoreComp;
		}

		// Load and compile plain text into runnable bytecode
		void JinxScriptingSystem::InitComponent(JinxScriptComponent& comp)
		{
			// Open File
			std::ifstream scriptFile;
			scriptFile.open(comp.Dir.c_str());

			std::string text, line;
			// If File opened successfully
			if (scriptFile.is_open())
			{
				// Write each line of file to text string
				while (getline(scriptFile, line))
				{
					text += line + '\n';
				}
			}

			// Compile text to bytecode and store in component BufferPtr;
			comp.Bytecode = runtime->Compile(text.c_str(), comp.Name.c_str(), { "puffin-core" });
			if (!comp.Bytecode)
			{
				std::cout << "Failed to compile bytecode for " << comp.Name.c_str() << std::endl;
			}
			else
			{
				// Create Runtime Script Object if bytecode compiled successfully
				comp.Script = runtime->CreateScript(comp.Bytecode);
			}
		}
	}
}