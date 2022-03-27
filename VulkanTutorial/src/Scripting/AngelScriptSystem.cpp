#include "AngelScriptSystem.h"

#include <iostream>  // cout
#include <assert.h>  // assert()
#include <string.h>
#include <windows.h> // timeGetTime()
#include <vector>

using namespace std;

void MessageCallback(const asSMessageInfo* msg, void* param)
{
	const char* type = "ERR ";
	if (msg->type == asMSGTYPE_WARNING)
		type = "WARN";
	else if (msg->type == asMSGTYPE_INFORMATION)
		type = "INFO";

	printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
}

// Function implementation with native calling convention
void PrintString(string& str)
{
	cout << str << endl;
}

// Function implementation with generic script interface
void PrintString_Generic(asIScriptGeneric* gen)
{
	string* str = (string*)gen->GetArgAddress(0);
	cout << *str << endl;
}

namespace Puffin
{
	namespace Scripting
	{
		void AngelScriptSystem::Init()
		{
			// Create Script Engine
			scriptEngine = asCreateScriptEngine();
			if (scriptEngine == 0)
			{
				cout << "Failed to create script engine." << endl;
			}

			// Set message callback to receive information on errors in human readable form
			int r = scriptEngine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL); assert(r >= 0);

			ConfigureEngine();
		}

		void AngelScriptSystem::Start()
		{
			// Create a context that will execute the script
			ctx = scriptEngine->CreateContext();
			if (ctx == 0)
			{
				cout << "Failed to create the context." << endl;
				scriptEngine->Release();
			}

			// Compile Scripts and Execute Start Methods
			for (ECS::Entity entity : entityMap["Script"])
			{
				AngelScriptComponent& script = m_world->GetComponent<AngelScriptComponent>(entity);
				InitScriptComponent(script);

				ExecuteScriptMethod(script.obj, script.startFunc);
			}
		}

		void AngelScriptSystem::Update()
		{
			// Initialize/Cleanup marked components
			for (ECS::Entity entity : entityMap["Script"])
			{
				AngelScriptComponent& script = m_world->GetComponent<AngelScriptComponent>(entity);

				// Script needs initialized
				if (!m_world->ComponentInitialized<AngelScriptComponent>(entity))
				{
					InitScriptComponent(script);
					m_world->SetComponentInitialized<AngelScriptComponent>(entity, true);
					ExecuteScriptMethod(script.obj, script.startFunc);
				}

				// Script needs cleaned up
				if (m_world->ComponentDeleted<AngelScriptComponent>(entity) || m_world->IsDeleted(entity))
				{
					ExecuteScriptMethod(script.obj, script.stopFunc);
					CleanupScriptComponent(script);
					m_world->RemoveComponent<AngelScriptComponent>(entity);
				}
			}

			// Execute Scripts Update Method
			for (ECS::Entity entity : entityMap["Script"])
			{
				AngelScriptComponent& script = m_world->GetComponent<AngelScriptComponent>(entity);

				// Execute Update function if one was found for script
				ExecuteScriptMethod(script.obj, script.updateFunc);
			}
		}

		void AngelScriptSystem::Stop()
		{
			// Execute Script Stop Methods
			for (ECS::Entity entity : entityMap["Script"])
			{
				AngelScriptComponent& script = m_world->GetComponent<AngelScriptComponent>(entity);
				ExecuteScriptMethod(script.obj, script.stopFunc);
			}

			// We must release the contexts when no longer using them
			if (ctx)
			{
				ctx->Release();
			}

			for (ECS::Entity entity : entityMap["Script"])
			{
				AngelScriptComponent& script = m_world->GetComponent<AngelScriptComponent>(entity);
				CleanupScriptComponent(script);
			}
		}

		void AngelScriptSystem::Cleanup()
		{
			// Shut down the engine
			scriptEngine->ShutDownAndRelease();
			scriptEngine = nullptr;
		}

		void AngelScriptSystem::ConfigureEngine()
		{
			int r;

			// Register the script string type
			// Look at the implementation for this function for more information  
			// on how to register a custom string type, and other object types.
			RegisterStdString(scriptEngine);

			if (!strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY"))
			{
				// Register the functions that the scripts will be allowed to use.
				// Note how the return code is validated with an assert(). This helps
				// us discover where a problem occurs, and doesn't pollute the code
				// with a lot of if's. If an error occurs in release mode it will
				// be caught when a script is being built, so it is not necessary
				// to do the verification here as well.
				r = scriptEngine->RegisterGlobalFunction("void Print(string &in)", asFUNCTION(PrintString), asCALL_CDECL); assert(r >= 0);
			}
			else
			{
				// Notice how the registration is almost identical to the above. 
				r = scriptEngine->RegisterGlobalFunction("void Print(string &in)", asFUNCTION(PrintString_Generic), asCALL_GENERIC); assert(r >= 0);
			}

			// Define Global Functions for Scripts
			r = scriptEngine->RegisterGlobalFunction("double GetDeltaTime()", asMETHOD(AngelScriptSystem, GetDeltaTime), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
			r = scriptEngine->RegisterGlobalFunction("double GetFixedTime()", asMETHOD(AngelScriptSystem, GetFixedTime), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);

			// It is possible to register the functions, properties, and types in 
			// configuration groups as well. When compiling the scripts it then
			// be defined which configuration groups should be available for that
			// script. If necessary a configuration group can also be removed from
			// the engine, so that the engine configuration could be changed 
			// without having to recompile all the scripts.
		}

		void AngelScriptSystem::InitScriptComponent(AngelScriptComponent& script)
		{
			// Compile the script into a module
			int r;

			// The builder is a helper class that will load the script file, 
			// search for #include directives, and load any included files as 
			// well.
			CScriptBuilder builder;

			// Build the script. If there are any compiler messages they will
			// be written to the message stream that we set right after creating the 
			// script engine. If there are no errors, and no warnings, nothing will
			// be written to the stream.
			r = builder.StartNewModule(scriptEngine, script.name.c_str());
			if (r < 0)
			{
				cout << "Failed to start new module" << endl;
			}
			r = builder.AddSectionFromFile(script.dir.string().c_str());
			if (r < 0)
			{
				cout << "Failed to add script file" << endl;
			}
			r = builder.BuildModule();
			if (r < 0)
			{
				cout << "Failed to build the module" << endl;
			}
			else
			{
				// Get the object type from the compiled module
				asIScriptModule* mod = scriptEngine->GetModule(script.name.c_str());

				asITypeInfo* typeToInstantiate = nullptr;

				// Iterate over all types in this module
				int objectCount = mod->GetObjectTypeCount();
				for (int n = 0; n < objectCount; n++)
				{
					asITypeInfo* type = mod->GetObjectTypeByIndex(n);
					std::vector<std::string> metadata = builder.GetMetadataForType(type->GetTypeId());

					// Check each types metadata
					for (int m = 0; m < metadata.size(); m++)
					{
						// If the type has metadata "instantiate" that means it should have an object ptr created
						if (metadata[m] == "Instantiate")
						{
							typeToInstantiate = type;
						}
					}
				}

				// If a type to instantiate has been found
				if (typeToInstantiate != nullptr)
				{
					if (typeToInstantiate->GetFactoryCount() > 0)
					{
						// Create the type using its factory function
						asIScriptFunction* factory = typeToInstantiate->GetFactoryByIndex(0);

						// Prepare context to call factory function
						ctx->Prepare(factory);

						// Execute Call
						ctx->Execute();

						// Get created object, and increase its reference count by one
						script.obj = *(asIScriptObject**)ctx->GetAddressOfReturnValue();
						script.obj->AddRef();

						// Store type interface for later
						script.type = typeToInstantiate;
						script.type->AddRef();

						// Get Common Functions, if they are defined

						// Start
						script.startFunc = GetScriptMethod(script, "Start");

						// Update
						script.updateFunc = GetScriptMethod(script, "Update");

						// Stop
						script.stopFunc = GetScriptMethod(script, "Stop");
					}
					else
					{
						cout << "No factory function existed for this class" << endl;
					}

					// Get all editable/visible properties
					int propertyCount = typeToInstantiate->GetPropertyCount();
					for (int p = 0; p < propertyCount; p++)
					{
						std::vector<std::string> metadata = builder.GetMetadataForTypeProperty(typeToInstantiate->GetTypeId(), p);

						// Check each properties metadata
						for (int m = 0; m < metadata.size(); m++)
						{
							if (metadata[m] == "Editable")
							{
								script.editableProperties.insert(p);
							}

							if (metadata[m] == "Visible")
							{
								script.visibleProperties.insert(p);
							}
						}
					}
				}
			}

			// The engine doesn't keep a copy of the script sections after Build() has
			// returned. So if the script needs to be recompiled, then all the script
			// sections must be added again.

			// If we want to have several scripts executing at different times but 
			// that have no direct relation with each other, then we can compile them
			// into separate script modules. Each module use their own namespace and 
			// scope, so function names, and global variables will not conflict with
			// each other.
		}

		asIScriptFunction* AngelScriptSystem::GetScriptMethod(const AngelScriptComponent& script, const char* funcName)
		{
			asIScriptFunction* scriptFunc = script.type->GetMethodByName(funcName);

			if (scriptFunc != 0)
			{
				scriptFunc->AddRef();
			}
			else
			{
				cout << "Failed to find " << funcName << " method for " << script.name << endl;
			}

			return scriptFunc;
		}

		void AngelScriptSystem::CleanupScriptComponent(AngelScriptComponent& script)
		{
			script.type->Release();
			script.obj->Release();
			script.startFunc->Release();
			script.updateFunc->Release();
			script.stopFunc->Release();
			script.editableProperties.clear();
			script.visibleProperties.clear();
		}

		bool AngelScriptSystem::ExecuteScriptMethod(asIScriptObject* scriptObj, asIScriptFunction* scriptFunc)
		{
			if (scriptObj != 0 && scriptFunc != 0)
			{
				// Prepare Function for execution
				ctx->Prepare(scriptFunc);

				// Set Object pointer
				ctx->SetObject(scriptObj);

				// Execute the function
				int r = ctx->Execute();
				if (r != asEXECUTION_FINISHED)
				{
					// The execution didn't finish as we had planned. Determine why.
					if (r == asEXECUTION_ABORTED)
						cout << "The script was aborted before it could finish. Probably it timed out." << endl;
					else if (r == asEXECUTION_EXCEPTION)
					{
						cout << "The script ended with an exception." << endl;

						// Write some information about the script exception
						asIScriptFunction* func = ctx->GetExceptionFunction();
						cout << "func: " << func->GetDeclaration() << endl;
						cout << "modl: " << func->GetModuleName() << endl;
						//cout << "sect: " << func->GetScriptSectionName() << endl;
						cout << "line: " << ctx->GetExceptionLineNumber() << endl;
						cout << "desc: " << ctx->GetExceptionString() << endl;
					}
					else
						cout << "The script ended for some unforeseen reason (" << r << ")." << endl;

					return false;
				}

				return true;
			}

			cout << "Either the Script Object or Function was null" << endl;
			return false;
		}

		// Global Script Functions

		double AngelScriptSystem::GetDeltaTime()
		{
			return m_deltaTime;
		}

		double AngelScriptSystem::GetFixedTime()
		{
			return m_fixedTime;
		}
	}
}