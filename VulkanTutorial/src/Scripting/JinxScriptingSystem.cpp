#include "JinxScriptingSystem.h"

namespace Puffin
{
	namespace Scripting
	{
		// Public Functions
		void JinxScriptingSystem::Init()
		{
		   // Create Jinx runtime object
			runtime = Jinx::CreateRuntime();
	
			// Text containing Jinx Script
			const char* scriptText =
			u8R"(
			-- Use the core library
			import core
	
			-- Write to debug output
			write line "Hello, Jinx World!"
	
			loop x from 1 to 10
				write line "x = ", x
			end
			
			)";
	
			// Create and execute a script object
			auto script = runtime->ExecuteScript(scriptText);

			// Scripts will be compiled here to be executed during runtime
			const char* runtimeText = 
			u8R"(
			
			-- Use the core library
			import core

			external dt

			write line "Delta Time: ", dt

			)";

			bytecode = runtime->Compile(runtimeText);
		}
	
		bool JinxScriptingSystem::Update(float dt)
		{
			auto script = runtime->CreateScript(bytecode);
			script->SetVariable("dt", dt);
			script->Execute();

			return true;
		}
	
		void JinxScriptingSystem::Cleanup()
		{
	
		}
	
		// Private Functions
	}
}