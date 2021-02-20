#include "JinxScriptingSystem.h"
#include <fstream>
#include <iostream>

namespace Puffin
{
	namespace Scripting
	{
		// Public Functions
		void JinxScriptingSystem::Init()
		{
			// Create Jinx runtime object
			runtime = Jinx::CreateRuntime();

			// Compile Bytecode for script components
			for (ECS::Entity entity : entityMap["Script"])
			{
				JinxScriptComponent& comp = world->GetComponent<JinxScriptComponent>(entity);
				InitComponent(comp);
			}
		}

		bool JinxScriptingSystem::Update(float dt)
		{
			for (ECS::Entity entity : entityMap["Script"])
			{
				JinxScriptComponent& comp = world->GetComponent<JinxScriptComponent>(entity);

				//comp.Script = runtime->CreateScript(comp.Bytecode);
				comp.Script->SetVariable("delta_time", dt);
				comp.Script->Execute();
			}

			return true;
		}

		void JinxScriptingSystem::Cleanup()
		{

		}

		// Private Functions

		// Load and compile plain text into runnable bytecode
		void JinxScriptingSystem::InitComponent(JinxScriptComponent& comp)
		{
			// Open File
			std::ifstream scriptFile;
			scriptFile.open(comp.Name.c_str());

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
			comp.Bytecode = runtime->Compile(text.c_str());
			if (!comp.Bytecode)
			{
				std::cout << "Failed to compile bytecode\n";
			}

			//comp.Script = runtime->CreateScript(comp.Bytecode);
		}
	}
}