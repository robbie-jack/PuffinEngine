#include "AngelScriptSystem.h"

#include "RegisterTypeHelpers.h"
#include "Types/ComponentFlags.h"

#include "Assets/AssetRegistry.h"

#include "Engine/Engine.hpp"
#include "Engine/EventSubsystem.hpp"

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

namespace Puffin::Scripting
{
	AngelScriptSystem::AngelScriptSystem()
	{
		// Create Script Engine
		m_scriptEngine = asCreateScriptEngine();
		if (m_scriptEngine == 0)
		{
			cout << "Failed to create script engine." << endl;
		}

		// Set message callback to receive information on errors in human readable form
		int r = m_scriptEngine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL); assert(r >= 0);

		m_systemInfo.name = "AngelScriptSystem";
		m_systemInfo.updateOrder = Core::UpdateOrder::Update;
	}

	AngelScriptSystem::~AngelScriptSystem()
	{
		// Shut down the engine
		m_scriptEngine->ShutDownAndRelease();
		m_scriptEngine = nullptr;
	}

	void AngelScriptSystem::Init()
	{
		// Configure Engine and Setup Global Function Callbacks
		ConfigureEngine();

		auto eventSubsystem = m_engine->GetSubsystem<Core::EventSubsystem>();

		// Subscribe to events
		m_inputEvents = std::make_shared<RingBuffer<Input::InputEvent>>();
		eventSubsystem->Subscribe<Input::InputEvent>(m_inputEvents);

		m_collisionBeginEvents = std::make_shared<RingBuffer<Physics::CollisionBeginEvent>>();
		eventSubsystem->Subscribe<Physics::CollisionBeginEvent>(m_collisionBeginEvents);

		m_collisionEndEvents = std::make_shared<RingBuffer<Physics::CollisionEndEvent>>();
		eventSubsystem->Subscribe<Physics::CollisionEndEvent>(m_collisionEndEvents);

		m_audioSubsystem = m_engine->GetSubsystem<Audio::AudioSubsystem>();
	}

	void AngelScriptSystem::PreStart()
	{
		// Create a context that will execute the script
		m_ctx = m_scriptEngine->CreateContext();
		if (m_ctx == 0)
		{
			cout << "Failed to create the context." << endl;
			m_scriptEngine->Release();
		}

		// Compile Scripts/Instantiate Objects
		for (ECS::EntityID entity : entityMap["Script"])
		{
			auto& script = m_world->GetComponent<AngelScriptComponent>(entity);

			InitializeScript(entity, script);

			m_world->SetComponentFlag<AngelScriptComponent, FlagDirty>(entity, false);
		}
	}

	void AngelScriptSystem::Start()
	{
		// Execute Start Methods
		for (ECS::EntityID entity : entityMap["Script"])
		{
			auto& script = m_world->GetComponent<AngelScriptComponent>(entity);

			ExportEditablePropertiesToScriptData(script, script.serializedData);

			m_currentEntityID = entity;

			asIScriptFunction* startFunc = GetScriptMethod(script, "Start");
			PrepareAndExecuteScriptMethod(script.obj, startFunc);
			startFunc->Release();
		}
	}

	void AngelScriptSystem::Update()
	{
		// Process Input Events
		ProcessEvents();
		
		// Initialize/Cleanup marked components
		for (ECS::EntityID entity : entityMap["Script"])
		{
			auto& script = m_world->GetComponent<AngelScriptComponent>(entity);

			m_currentEntityID = entity;

			// Script needs initialized
			if (m_world->GetComponentFlag<AngelScriptComponent, FlagDirty>(entity))
			{
				InitializeScript(entity, script);

				PrepareAndExecuteScriptMethod(script.obj, script.startFunc);

				m_world->SetComponentFlag<AngelScriptComponent, FlagDirty>(entity, false);
			}

			// Script needs cleaned up
			if (m_world->GetComponentFlag<AngelScriptComponent, FlagDeleted>(entity) || m_world->GetEntityFlag<FlagDeleted>(entity))
			{
				PrepareAndExecuteScriptMethod(script.obj, script.stopFunc);

				CleanupScriptComponent(script);
				m_world->RemoveComponent<AngelScriptComponent>(entity);
			}

			// Execute Update function if one was found for script
			PrepareAndExecuteScriptMethod(script.obj, script.updateFunc);
		}
	}

	void AngelScriptSystem::Stop()
	{
		// Execute Script Stop Methods
		for (ECS::EntityID entity : entityMap["Script"])
		{
			auto& script = m_world->GetComponent<AngelScriptComponent>(entity);

			m_currentEntityID = entity;

			PrepareAndExecuteScriptMethod(script.obj, script.stopFunc);
		}

		// Release Input Pressed Callbacks
		for (auto& pair1 : m_onInputPressedCallbacks)
		{
			for (auto& pair2 : pair1.second)
			{
				ReleaseCallback(pair2.second);
			}
		}

		m_onInputPressedCallbacks.clear();

		// Release Input Released Callbacks
		for (auto& pair1 : m_onInputReleasedCallbacks)
		{
			for (auto& pair2 : pair1.second)
			{
				ReleaseCallback(pair2.second);
			}
		}

		m_onInputPressedCallbacks.clear();

		// Release collision begin callbacks
		for (auto& pair : m_onCollisionBeginCallbacks)
		{
			ReleaseCallback(pair.second);
		}

		m_onCollisionBeginCallbacks.clear();

		// Release collision end callbacks
		for (auto& pair : m_onCollisionEndCallbacks)
		{
			ReleaseCallback(pair.second);
		}

		m_onCollisionEndCallbacks.clear();

		// We must release the contexts when no longer using them
		if (m_ctx)
		{
			m_ctx->Release();
		}

		for (ECS::EntityID entity : entityMap["Script"])
		{
			auto& script = m_world->GetComponent<AngelScriptComponent>(entity);
			CleanupScriptComponent(script);
		}
	}

	void AngelScriptSystem::ConfigureEngine()
	{
		int r;

		// Register the script string type
		// Look at the implementation for this function for more information  
		// on how to register a custom string type, and other object types.
		RegisterStdString(m_scriptEngine);

		if (!strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY"))
		{
			// Register the functions that the scripts will be allowed to use.
			// Note how the return code is validated with an assert(). This helps
			// us discover where a problem occurs, and doesn't pollute the code
			// with a lot of if's. If an error occurs in release mode it will
			// be caught when a script is being built, so it is not necessary
			// to do the verification here as well.
			r = m_scriptEngine->RegisterGlobalFunction("void Print(string &in)", asFUNCTION(PrintString), asCALL_CDECL); assert(r >= 0);
		}
		else
		{
			// Notice how the registration is almost identical to the above. 
			r = m_scriptEngine->RegisterGlobalFunction("void Print(string &in)", asFUNCTION(PrintString_Generic), asCALL_GENERIC); assert(r >= 0);
		}

		// Define Global Methods for Scripts
		r = m_scriptEngine->RegisterGlobalFunction("double GetDeltaTime()", asMETHOD(AngelScriptSystem, GetDeltaTime), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = m_scriptEngine->RegisterGlobalFunction("double GetFixedTime()", asMETHOD(AngelScriptSystem, GetFixedTime), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = m_scriptEngine->RegisterGlobalFunction("uint GetEntityID()", asMETHOD(AngelScriptSystem, GetEntityID), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);

		r = m_scriptEngine->RegisterGlobalFunction("void PlaySoundEffect(uint64, float, bool, bool)", asMETHODPR(AngelScriptSystem, PlaySoundEffect, (uint64_t, float, bool, bool), void), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = m_scriptEngine->RegisterGlobalFunction("uint64 PlaySoundEffect(const string &in, float, bool, bool)", asMETHODPR(AngelScriptSystem, PlaySoundEffect, (const string&, float, bool, bool), uint64_t), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);

		// Register Components and their constructors, functions and properties
		RegisterTransformComponent(m_scriptEngine, m_world.get());

		// Register Input Funcdefs and Bind/Release Callback Methods
		r = m_scriptEngine->RegisterFuncdef("void OnInputPressedCallback()"); assert(r >= 0);
		r = m_scriptEngine->RegisterFuncdef("void OnInputReleasedCallback()"); assert(r >= 0);
		
		r = m_scriptEngine->RegisterGlobalFunction("void BindOnInputPressed(uint, const string &in, OnInputPressedCallback @cb)", asMETHOD(AngelScriptSystem, BindOnInputPressed), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = m_scriptEngine->RegisterGlobalFunction("void BindOnInputReleased(uint, const string &in, OnInputReleasedCallback @cb)", asMETHOD(AngelScriptSystem, BindOnInputReleased), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = m_scriptEngine->RegisterGlobalFunction("void ReleaseOnInputPressed(uint, const string &in)", asMETHOD(AngelScriptSystem, ReleaseOnInputPressed), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = m_scriptEngine->RegisterGlobalFunction("void ReleaseOnInputReleased(uint, const string &in)", asMETHOD(AngelScriptSystem, ReleaseOnInputReleased), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);

		// Register Collision Funcdefs and Bind/Release Callback Methods
		r = m_scriptEngine->RegisterFuncdef("void OnCollisionBeginCallback(uint)"); assert(r >= 0);
		r = m_scriptEngine->RegisterFuncdef("void OnCollisionEndCallback(uint)"); assert(r >= 0);

		r = m_scriptEngine->RegisterGlobalFunction("void BindOnCollisionBegin(uint, OnCollisionBeginCallback @cb)", asMETHOD(AngelScriptSystem, BindOnCollisionBegin), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = m_scriptEngine->RegisterGlobalFunction("void BindOnCollisionEnd(uint, OnCollisionEndCallback @cb)", asMETHOD(AngelScriptSystem, BindOnCollisionEnd), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = m_scriptEngine->RegisterGlobalFunction("void ReleaseOnCollisionBegin(uint)", asMETHOD(AngelScriptSystem, ReleaseOnCollisionBegin), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = m_scriptEngine->RegisterGlobalFunction("void ReleaseOnCollisionEnd(uint)", asMETHOD(AngelScriptSystem, ReleaseOnCollisionEnd), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);

		// It is possible to register the functions, properties, and types in 
		// configuration groups as well. When compiling the scripts it then
		// be defined which configuration groups should be available for that
		// script. If necessary a configuration group can also be removed from
		// the engine, so that the engine configuration could be changed 
		// without having to recompile all the scripts.
	}

	void AngelScriptSystem::InitializeScript(ECS::EntityID entity, AngelScriptComponent& script)
	{
		CompileScript(script);
		UpdateScriptMethods(script);
		InstantiateScriptObj(entity, script);
		ImportEditableProperties(script, script.serializedData);
	}

	void AngelScriptSystem::CompileScript(AngelScriptComponent& script)
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
		r = builder.StartNewModule(m_scriptEngine, script.name.c_str());
		if (r < 0)
		{
			cout << "Failed to start new module" << endl;
		}

		fs::path scriptPath = Assets::AssetRegistry::Get()->ContentRoot() / script.dir;

		r = builder.AddSectionFromFile(scriptPath.string().c_str());
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
			asIScriptModule* mod = m_scriptEngine->GetModule(script.name.c_str());

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
					script.obj = nullptr;

					// Store type interface for later
					script.type = typeToInstantiate;
					script.type->AddRef();
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
						bool isEditable = false;

						if (metadata[m] == "Editable")
						{
							script.editableProperties.insert(p);
							isEditable = true;
						}

						if (metadata[m] == "Visible" && !isEditable)
						{
							script.visibleProperties.insert(p);
						}
					}
				}
			}
		}
	}

	void AngelScriptSystem::UpdateScriptMethods(AngelScriptComponent& script)
	{
		if (script.type != 0)
		{
			script.startFunc = GetScriptMethod(script, "Start");
			script.updateFunc = GetScriptMethod(script, "Update");
			script.stopFunc = GetScriptMethod(script, "Stop");
		}
	}

	void AngelScriptSystem::InstantiateScriptObj(ECS::EntityID entity, AngelScriptComponent& script)
	{
		if (script.type != 0 && script.type->GetFactoryCount() > 0)
		{
			// Create the type using its factory function
			asIScriptFunction* factory = script.type->GetFactoryByIndex(0);

			// Prepare context to call factory function
			m_ctx->Prepare(factory);

			// Execute Call
			m_ctx->Execute();

			// Get created object if nullptr, and increase its reference count by one
			script.obj = *(asIScriptObject**)m_ctx->GetAddressOfReturnValue();
			script.obj->AddRef();
		}
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

		if (script.obj != nullptr)
		{
			script.obj->Release();
		}

		script.editableProperties.clear();
		script.visibleProperties.clear();
	}

	void AngelScriptSystem::ProcessEvents()
	{
		// Process Input Events
		Input::InputEvent inputEvent;
		while (m_inputEvents->Pop(inputEvent))
		{
			if (m_onInputPressedCallbacks.count(inputEvent.actionName) && inputEvent.actionState == Puffin::Input::KeyState::PRESSED)
			{
				for (const auto& pair : m_onInputPressedCallbacks[inputEvent.actionName])
				{
					const ScriptCallback& callback = pair.second;

					PrepareAndExecuteScriptMethod(callback.object, callback.func);
				}
			}

			if (m_onInputReleasedCallbacks.count(inputEvent.actionName) && inputEvent.actionState == Puffin::Input::KeyState::RELEASED)
			{
				for (const auto& pair : m_onInputReleasedCallbacks[inputEvent.actionName])
				{
					const ScriptCallback& callback = pair.second;

					PrepareAndExecuteScriptMethod(callback.object, callback.func);
				}
			}
		}

		// Process Collision Begin Events
		Physics::CollisionBeginEvent collisionBeginEvent;
		while (m_collisionBeginEvents->Pop(collisionBeginEvent))
		{
			// Invoke collision EntityA callback if one exists
			if (m_onCollisionBeginCallbacks.count(collisionBeginEvent.entityA) == 1)
			{
				const ScriptCallback& callbackA = m_onCollisionBeginCallbacks[collisionBeginEvent.entityA];

				// Prepare callback
				PrepareScriptMethod(callbackA.object, callbackA.func);

				// Bind entityB parameter
				m_ctx->SetArgDWord(0, collisionBeginEvent.entityB);

				// Execute callback
				ExecuteScriptMethod(callbackA.object, callbackA.func);
			}

			// Invoke collision EntityB callback if one exists
			if (m_onCollisionBeginCallbacks.count(collisionBeginEvent.entityB) == 1)
			{
				const ScriptCallback& callbackB = m_onCollisionBeginCallbacks[collisionBeginEvent.entityB];

				// Prepare callback
				PrepareScriptMethod(callbackB.object, callbackB.func);

				// Bind entityA parameter
				m_ctx->SetArgDWord(0, collisionBeginEvent.entityA);

				// Execute callback
				ExecuteScriptMethod(callbackB.object, callbackB.func);
			}
		}

		// Process Collision End Events
		Physics::CollisionBeginEvent collisionEndEvent;
		while (m_collisionEndEvents->Pop(collisionEndEvent))
		{
			// Invoke collision EntityA callback if one exists
			if (m_onCollisionEndCallbacks.count(collisionEndEvent.entityA) == 1)
			{
				const ScriptCallback& callbackA = m_onCollisionEndCallbacks[collisionEndEvent.entityA];

				// Prepare callback
				PrepareScriptMethod(callbackA.object, callbackA.func);

				// Bind entityB parameter
				m_ctx->SetArgDWord(0, collisionEndEvent.entityB);

				// Execute callback
				ExecuteScriptMethod(callbackA.object, callbackA.func);
			}

			// Invoke collision EntityB callback if one exists
			if (m_onCollisionEndCallbacks.count(collisionEndEvent.entityB) == 1)
			{
				const ScriptCallback& callbackB = m_onCollisionEndCallbacks[collisionEndEvent.entityB];

				// Prepare callback
				PrepareScriptMethod(callbackB.object, callbackB.func);

				// Bind entityA parameter
				m_ctx->SetArgDWord(0, collisionEndEvent.entityA);

				// Execute callback
				ExecuteScriptMethod(callbackB.object, callbackB.func);
			}
		}
	}

	bool AngelScriptSystem::PrepareScriptMethod(void* scriptObj, asIScriptFunction* scriptFunc)
	{
		if (scriptObj != 0 && scriptFunc != 0)
		{
			// Prepare Function for execution
			m_ctx->Prepare(scriptFunc);

			// Set Object pointer
			m_ctx->SetObject(scriptObj);

			return true;
		}

		cout << "Either the Script Object or Function was null" << endl;
		return false;
	}

	bool AngelScriptSystem::ExecuteScriptMethod(void* scriptObj, asIScriptFunction* scriptFunc)
	{
		if (scriptObj != 0 && scriptFunc != 0)
		{
			// Execute the function
			int r = m_ctx->Execute();
			if (r != asEXECUTION_FINISHED)
			{
				// The execution didn't finish as we had planned. Determine why.
				if (r == asEXECUTION_ABORTED)
					cout << "The script was aborted before it could finish. Probably it timed out." << endl;
				else if (r == asEXECUTION_EXCEPTION)
				{
					cout << "The script ended with an exception." << endl;

					// Write some information about the script exception
					asIScriptFunction* func = m_ctx->GetExceptionFunction();
					cout << "func: " << func->GetDeclaration() << endl;
					cout << "modl: " << func->GetModuleName() << endl;
					//cout << "sect: " << func->GetScriptSectionName() << endl;
					cout << "line: " << m_ctx->GetExceptionLineNumber() << endl;
					cout << "desc: " << m_ctx->GetExceptionString() << endl;
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

	bool AngelScriptSystem::PrepareAndExecuteScriptMethod(void* scriptObj, asIScriptFunction* scriptFunc)
	{
		if (PrepareScriptMethod(scriptObj, scriptFunc))
		{
			return ExecuteScriptMethod(scriptObj, scriptFunc);
		}

		return false;
	}

	// Global Script Functions

	const double& AngelScriptSystem::GetDeltaTime() const
	{
		return m_engine->GetDeltaTime();
	}

	const double& AngelScriptSystem::GetFixedTime() const
	{
		return m_engine->GetTimeStep();
	}

	void AngelScriptSystem::PlaySoundEffect(uint64_t id, float volume, bool looping, bool restart)
	{
		if (m_audioSubsystem)
		{
			m_audioSubsystem->PlaySoundEffect(id, volume, looping, restart);
		}
	}

	uint64_t AngelScriptSystem::PlaySoundEffect(const std::string& path, float volume, bool looping, bool restart)
	{
		uint64_t id = 0;

		if (m_audioSubsystem)
		{
			id = m_audioSubsystem->PlaySoundEffect(path, volume, looping, restart);
		}

		return id;
	}

	int AngelScriptSystem::GetEntityID()
	{
		return m_currentEntityID;
	}

	ScriptCallback AngelScriptSystem::BindCallback(uint32_t entity, asIScriptFunction* cb) const
	{
		ScriptCallback scriptCallback;
		scriptCallback.entity = entity;

		if (cb && cb->GetFuncType() == asFUNC_DELEGATE)
		{
			// Store object, type and callback
			scriptCallback.object = cb->GetDelegateObject();
			scriptCallback.objectType = cb->GetDelegateObjectType();
			scriptCallback.func = cb->GetDelegateFunction();

			// Increment Refs
			m_scriptEngine->AddRefScriptObject(scriptCallback.object, scriptCallback.objectType);
			scriptCallback.func->AddRef();

			// Release Delegate
			cb->Release();
		}
		else
		{
			//Store handle for later use
			scriptCallback.func = cb;
		}

		return scriptCallback;
	}

	void AngelScriptSystem::ReleaseCallback(ScriptCallback& scriptCallback) const
	{
		if (scriptCallback.func)
			scriptCallback.func->Release();

		if (scriptCallback.object)
			m_scriptEngine->ReleaseScriptObject(scriptCallback.object, scriptCallback.objectType);

		scriptCallback.func = nullptr;
		scriptCallback.object = nullptr;
		scriptCallback.objectType = nullptr;
	}

	// Input Callbacks

	void AngelScriptSystem::BindOnInputPressed(uint32_t entity, const std::string& actionName, asIScriptFunction* cb)
	{
		// Release existing callback function, if one exists
		ReleaseOnInputPressed(entity, actionName);

		m_onInputPressedCallbacks[actionName][entity] = BindCallback(entity, cb);
	}

	void AngelScriptSystem::BindOnInputReleased(uint32_t entity, const std::string& actionName, asIScriptFunction* cb)
	{
		// Release existing callback function, if one exists
		ReleaseOnInputReleased(entity, actionName);

		m_onInputReleasedCallbacks[actionName][entity] = BindCallback(entity, cb);
	}

	void AngelScriptSystem::ReleaseOnInputPressed(uint32_t entity, const std::string& actionName)
	{
		if (m_onInputPressedCallbacks[actionName].count(entity))
		{
			ReleaseCallback(m_onInputPressedCallbacks[actionName][entity]);
			m_onInputPressedCallbacks[actionName].erase(entity);
		}
	}

	void AngelScriptSystem::ReleaseOnInputReleased(uint32_t entity, const std::string& actionName)
	{
		if (m_onInputReleasedCallbacks[actionName].count(entity))
		{
			ReleaseCallback(m_onInputReleasedCallbacks[actionName][entity]);
			m_onInputReleasedCallbacks[actionName].erase(entity);
		}
	}

	// Collision Callbacks

	void AngelScriptSystem::BindOnCollisionBegin(uint32_t entity, asIScriptFunction* cb)
	{
		ReleaseOnCollisionBegin(entity);

		m_onCollisionBeginCallbacks[entity] = BindCallback(entity, cb);
	}

	void AngelScriptSystem::BindOnCollisionEnd(uint32_t entity, asIScriptFunction* cb)
	{
		ReleaseOnCollisionEnd(entity);

		m_onCollisionEndCallbacks[entity] = BindCallback(entity, cb);
	}

	void AngelScriptSystem::ReleaseOnCollisionBegin(uint32_t entity)
	{
		if (m_onCollisionBeginCallbacks.count(entity) == 1)
		{
			ReleaseCallback(m_onCollisionBeginCallbacks[entity]);
			m_onCollisionBeginCallbacks.erase(entity);
		}
	}

	void AngelScriptSystem::ReleaseOnCollisionEnd(uint32_t entity)
	{
		if (m_onCollisionEndCallbacks.count(entity) == 1)
		{
			ReleaseCallback(m_onCollisionEndCallbacks[entity]);
			m_onCollisionEndCallbacks.erase(entity);
		}
	}
}