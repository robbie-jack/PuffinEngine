#include "puffin/scripting/angelscript/angelscriptsubsystem.h"

#include <assert.h>  // assert()
#include <iostream>  // cout
#include <string.h>
#include <vector>

#include "puffin/core/engine.h"
#include "puffin/assets/assetregistry.h"
#include "angelscript/scriptbuilder/scriptbuilder.h"
#include "angelscript/scriptstdstring/scriptstdstring.h"
#include "entt/entity/registry.hpp"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/scripting/angelscript/angelscriptengineinterface.h"
#include "puffin/scripting/angelscript/registertypehelpers.h"

using namespace std;

void messageCallback(const asSMessageInfo* msg, void* param)
{
	const char* type = "ERR ";
	if (msg->type == asMSGTYPE_WARNING)
		type = "WARN";
	else if (msg->type == asMSGTYPE_INFORMATION)
		type = "INFO";

	printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
}

// Function implementation with native calling convention
void printString(const string& str)
{
	cout << str << endl;
}

// Function implementation with generic script interface
void printStringGeneric(asIScriptGeneric* gen)
{
	const auto str = static_cast<string*>(gen->GetArgAddress(0));
	cout << *str << endl;
}

namespace puffin::scripting
{
	AngelScriptSubsystem::AngelScriptSubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
		mName = "AngelScriptSubsystem";
	}

	AngelScriptSubsystem::~AngelScriptSubsystem()
	{
		mEngineInterface = nullptr;

		// Shut down the engine
		mScriptEngine->ShutDownAndRelease();
		mScriptEngine = nullptr;

		mEngine = nullptr;
	}

	void AngelScriptSubsystem::Initialize(core::SubsystemManager* subsystemManager)
	{
		const auto enttSubsystem = subsystemManager->CreateAndInitializeSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		registry->on_construct<AngelScriptComponent>().connect<&AngelScriptSubsystem::OnConstructScript>(this);
		//registry->on_update<AngelScriptComponent>().connect<&AngelScriptSystem::onConstructScript>(this);
		registry->on_destroy<AngelScriptComponent>().connect<&AngelScriptSubsystem::OnDestroyScript>(this);

		// Create Script Engine
		mScriptEngine = asCreateScriptEngine();
		if (mScriptEngine == nullptr)
		{
			cout << "Failed to create script engine." << endl;
		}

		// Set message callback to receive information on errors in human readable form
		const int r = mScriptEngine->SetMessageCallback(asFUNCTION(messageCallback), nullptr, asCALL_CDECL); assert(r >= 0 && "Failed to set message callback for angelscript");

		// Configure Engine and Setup Global Function Callbacks
		ConfigureEngine();

		mEngineInterface = std::make_unique<AngelScriptEngineInterface>(mEngine, mScriptEngine);

		//mAudioSubsystem = mEngine->getSystem<audio::AudioSubsystem>();

		InitContext();
		InitScripts();
	}

	void AngelScriptSubsystem::Deinitialize()
	{
		
	}

	void AngelScriptSubsystem::EndPlay()
	{
		// Execute Script Stop Methods
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		const auto scriptView = registry->view<AngelScriptComponent>();

		for (auto [entity, script] : scriptView.each())
		{
			mCurrentEntityID = enttSubsystem->GetID(entity);

			DestroyScript(script);
		}

		// Release Input JustPressed Callbacks
		for (auto& [string, callbacks] : mOnInputPressedCallbacks)
		{
			for (auto& [id, callback] : callbacks)
			{
				ReleaseCallback(callback);
			}
		}

		mOnInputPressedCallbacks.clear();

		// Release Input JustReleased Callbacks
		for (auto& [string, callbacks] : mOnInputReleasedCallbacks)
		{
			for (auto& [id, callback] : callbacks)
			{
				ReleaseCallback(callback);
			}
		}

		mOnInputPressedCallbacks.clear();

		// Release collision begin callbacks
		for (auto& [id, callback] : mOnCollisionBeginCallbacks)
		{
			ReleaseCallback(callback);
		}

		mOnCollisionBeginCallbacks.clear();

		// Release collision end callbacks
		for (auto& [id, callback] : mOnCollisionEndCallbacks)
		{
			ReleaseCallback(callback);
		}

		mOnCollisionEndCallbacks.clear();

		// We must release the contexts when no longer using them
		if (mScriptContext)
		{
			mScriptContext->Release();
		}

		InitContext();
		InitScripts();
	}

	void AngelScriptSubsystem::Update(double deltaTime)
	{
		// Process Input Events
		ProcessEvents();

		// Destroy old scripts
		StopScripts();

		// Initialize new scripts
		InitScripts();

		// Run new scripts start method
		StartScripts();
	}

	bool AngelScriptSubsystem::ShouldUpdate()
	{
		return true;
	}

	void AngelScriptSubsystem::OnConstructScript(entt::registry& registry, entt::entity entity)
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto& id = enttSubsystem->GetID(entity);

		mScriptsToInit.emplace(id);
	}

	void AngelScriptSubsystem::OnDestroyScript(entt::registry& registry, entt::entity entity)
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto& id = enttSubsystem->GetID(entity);

		mScriptsToEndPlay.emplace(id);
	}

	void AngelScriptSubsystem::ConfigureEngine()
	{
		int r;

		// Register the script string type
		// Look at the implementation for this function for more information  
		// on how to register a custom string type, and other object types.
		RegisterStdString(mScriptEngine);

		if (!strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY"))
		{
			// Register the functions that the scripts will be allowed to use.
			// Note how the return code is validated with an assert(). This helps
			// us discover where a problem occurs, and doesn't pollute the code
			// with a lot of if's. If an error occurs in release mode it will
			// be caught when a script is being built, so it is not necessary
			// to do the verification here as well.
			r = mScriptEngine->RegisterGlobalFunction("void Print(string &in)", asFUNCTION(printString), asCALL_CDECL); assert(r >= 0);
		}
		else
		{
			// Notice how the registration is almost identical to the above. 
			r = mScriptEngine->RegisterGlobalFunction("void Print(string &in)", asFUNCTION(printStringGeneric), asCALL_GENERIC); assert(r >= 0);
		}

		// Define Global Methods for Scripts
		r = mScriptEngine->RegisterGlobalFunction("uint64 GetEntityID()", asMETHOD(AngelScriptSubsystem, GetEntityID), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);

		r = mScriptEngine->RegisterGlobalFunction("void PlaySoundEffect(uint64, float, bool, bool)", asMETHODPR(AngelScriptSubsystem, PlaySoundEffect, (uint64_t, float, bool, bool), void), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = mScriptEngine->RegisterGlobalFunction("uint64 PlaySoundEffect(const string &in, float, bool, bool)", asMETHODPR(AngelScriptSubsystem, PlaySoundEffect, (const string&, float, bool, bool), uint64_t), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);

		// Register Components and their constructors, functions and properties
		RegisterTransformComponent(mScriptEngine);

		// Register Collision Funcdefs and Bind/Release Callback Methods
		r = mScriptEngine->RegisterFuncdef("void OnCollisionBeginCallback(uint64)"); assert(r >= 0);
		r = mScriptEngine->RegisterFuncdef("void OnCollisionEndCallback(uint64)"); assert(r >= 0);

		r = mScriptEngine->RegisterGlobalFunction("void BindOnCollisionBegin(uint64, OnCollisionBeginCallback @cb)", asMETHOD(AngelScriptSubsystem, BindOnCollisionBegin), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = mScriptEngine->RegisterGlobalFunction("void BindOnCollisionEnd(uint64, OnCollisionEndCallback @cb)", asMETHOD(AngelScriptSubsystem, BindOnCollisionEnd), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = mScriptEngine->RegisterGlobalFunction("void ReleaseOnCollisionBegin(uint64)", asMETHOD(AngelScriptSubsystem, ReleaseOnCollisionBegin), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = mScriptEngine->RegisterGlobalFunction("void ReleaseOnCollisionEnd(uint64)", asMETHOD(AngelScriptSubsystem, ReleaseOnCollisionEnd), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);

		// It is possible to register the functions, properties, and types in 
		// configuration groups as well. When compiling the scripts it then
		// be defined which configuration groups should be available for that
		// script. If necessary a configuration group can also be removed from
		// the engine, so that the engine configuration could be changed 
		// without having to recompile all the scripts.
	}

	void AngelScriptSubsystem::InitContext()
	{
		mScriptContext = mScriptEngine->CreateContext();

		if (mScriptContext == nullptr)
		{
			cout << "Failed to create the context." << endl;
			mScriptEngine->Release();
		}
	}

	void AngelScriptSubsystem::InitScripts()
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		for (const auto id : mScriptsToInit)
		{
			entt::entity entity = enttSubsystem->GetEntity(id);

			auto& script = registry->get<AngelScriptComponent>(entity);

			InitializeScript(id, script);

			ExportEditablePropertiesToScriptData(script, script.serializedData);

			mScriptsToBeginPlay.emplace(id);
		}

		mScriptsToInit.clear();
	}

	void AngelScriptSubsystem::StartScripts()
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		for (const auto id : mScriptsToBeginPlay)
		{
			entt::entity entity = enttSubsystem->GetEntity(id);

			const auto& script = registry->get<AngelScriptComponent>(entity);

			mCurrentEntityID = id;

			PrepareAndExecuteScriptMethod(script.obj, script.beginPlayFunc);
		}

		mScriptsToBeginPlay.clear();
	}

	void AngelScriptSubsystem::StopScripts()
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		for (const auto id : mScriptsToEndPlay)
		{
			entt::entity entity = enttSubsystem->GetEntity(id);

			auto& script = registry->get<AngelScriptComponent>(entity);

			mCurrentEntityID = id;

			PrepareAndExecuteScriptMethod(script.obj, script.endPlayFunc);

			DestroyScript(script);
		}

		mScriptsToEndPlay.clear();
	}


	void AngelScriptSubsystem::InitializeScript(UUID entity, AngelScriptComponent& script)
	{
		CompileScript(script);
		UpdateScriptMethods(script);
		InstantiateScriptObj(entity, script);
		ImportEditableProperties(script, script.serializedData);
	}

	void AngelScriptSubsystem::CompileScript(AngelScriptComponent& script) const
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
		r = builder.StartNewModule(mScriptEngine, script.name.c_str());
		if (r < 0)
		{
			cout << "Failed to start new module" << endl;
		}

		const fs::path scriptPath = assets::AssetRegistry::Get()->GetContentRoot() / script.dir;

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
			asIScriptModule* mod = mScriptEngine->GetModule(script.name.c_str());

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
					for (auto& m : metadata)
					{
						bool isEditable = false;

						if (m == "Editable")
						{
							script.editableProperties.insert(p);
							isEditable = true;
						}

						if (m == "Visible" && !isEditable)
						{
							script.visibleProperties.insert(p);
						}
					}
				}
			}
		}
	}

	void AngelScriptSubsystem::UpdateScriptMethods(AngelScriptComponent& script)
	{
		if (script.type != nullptr)
		{
			script.beginPlayFunc = GetScriptMethod(script, "BeginPlay");
			script.fixedUpdateFunc = GetScriptMethod(script, "FixedUpdate");
			script.updateFunc = GetScriptMethod(script, "Update");
			script.endPlayFunc = GetScriptMethod(script, "EndPlay");
		}
	}

	void AngelScriptSubsystem::InstantiateScriptObj(UUID entity, AngelScriptComponent& script)
	{
		if (script.type != 0 && script.type->GetFactoryCount() > 0)
		{
			// Create the type using its factory function
			asIScriptFunction* factory = script.type->GetFactoryByIndex(0);

			// Prepare context to call factory function
			mScriptContext->Prepare(factory);

			// Execute Call
			mScriptContext->Execute();

			// Get created object if nullptr, and increase its reference count by one
			script.obj = *(asIScriptObject**)mScriptContext->GetAddressOfReturnValue();
			script.obj->AddRef();
		}
	}

	asIScriptFunction* AngelScriptSubsystem::GetScriptMethod(const AngelScriptComponent& script, const char* funcName)
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

	void AngelScriptSubsystem::DestroyScript(AngelScriptComponent& script)
	{
		script.type->Release();
		script.type = nullptr;

		if (script.obj != nullptr)
		{
			script.obj->Release();
			script.obj = nullptr;
		}

		script.editableProperties.clear();
		script.visibleProperties.clear();
	}

	void AngelScriptSubsystem::ProcessEvents()
	{
		// PUFFIN_TODO - Update input handling to work with signal subsystem

		// Process Input Events
		//input::InputEvent inputEvent;
		//while (m_inputEvents->Pop(inputEvent))
		//{
		//	if (m_onInputPressedCallbacks.count(inputEvent.actionName) && inputEvent.actionState == puffin::input::KeyState::JustPressed)
		//	{
		//		for (const auto& pair : m_onInputPressedCallbacks[inputEvent.actionName])
		//		{
		//			const ScriptCallback& callback = pair.second;

		//			PrepareAndExecuteScriptMethod(callback.object, callback.func);
		//		}
		//	}

		//	if (m_onInputReleasedCallbacks.count(inputEvent.actionName) && inputEvent.actionState == puffin::input::KeyState::JustReleased)
		//	{
		//		for (const auto& pair : m_onInputReleasedCallbacks[inputEvent.actionName])
		//		{
		//			const ScriptCallback& callback = pair.second;

		//			PrepareAndExecuteScriptMethod(callback.object, callback.func);
		//		}
		//	}
		//}

		//// Process Collision Begin Events
		//physics::CollisionBeginEvent collisionBeginEvent;
		//while (m_collisionBeginEvents->Pop(collisionBeginEvent))
		//{
		//	// Invoke collision EntityA callback if one exists
		//	if (m_onCollisionBeginCallbacks.count(collisionBeginEvent.entityA) == 1)
		//	{
		//		const ScriptCallback& callbackA = m_onCollisionBeginCallbacks[collisionBeginEvent.entityA];

		//		// Prepare callback
		//		PrepareScriptMethod(callbackA.object, callbackA.func);

		//		// Bind entityB parameter
		//		m_ctx->SetArgDWord(0, collisionBeginEvent.entityB);

		//		// Execute callback
		//		ExecuteScriptMethod(callbackA.object, callbackA.func);
		//	}

		//	// Invoke collision EntityB callback if one exists
		//	if (m_onCollisionBeginCallbacks.count(collisionBeginEvent.entityB) == 1)
		//	{
		//		const ScriptCallback& callbackB = m_onCollisionBeginCallbacks[collisionBeginEvent.entityB];

		//		// Prepare callback
		//		PrepareScriptMethod(callbackB.object, callbackB.func);

		//		// Bind entityA parameter
		//		m_ctx->SetArgDWord(0, collisionBeginEvent.entityA);

		//		// Execute callback
		//		ExecuteScriptMethod(callbackB.object, callbackB.func);
		//	}
		//}

		//// Process Collision End Events
		//physics::CollisionEndEvent collisionEndEvent;
		//while (m_collisionEndEvents->Pop(collisionEndEvent))
		//{
		//	// Invoke collision EntityA callback if one exists
		//	if (m_onCollisionEndCallbacks.count(collisionEndEvent.entityA) == 1)
		//	{
		//		const ScriptCallback& callbackA = m_onCollisionEndCallbacks[collisionEndEvent.entityA];

		//		// Prepare callback
		//		PrepareScriptMethod(callbackA.object, callbackA.func);

		//		// Bind entityB parameter
		//		m_ctx->SetArgDWord(0, collisionEndEvent.entityB);

		//		// Execute callback
		//		ExecuteScriptMethod(callbackA.object, callbackA.func);
		//	}

		//	// Invoke collision EntityB callback if one exists
		//	if (m_onCollisionEndCallbacks.count(collisionEndEvent.entityB) == 1)
		//	{
		//		const ScriptCallback& callbackB = m_onCollisionEndCallbacks[collisionEndEvent.entityB];

		//		// Prepare callback
		//		PrepareScriptMethod(callbackB.object, callbackB.func);

		//		// Bind entityA parameter
		//		m_ctx->SetArgDWord(0, collisionEndEvent.entityA);

		//		// Execute callback
		//		ExecuteScriptMethod(callbackB.object, callbackB.func);
		//	}
		//}
	}

	bool AngelScriptSubsystem::PrepareScriptMethod(void* scriptObj, asIScriptFunction* scriptFunc)
	{
		if (scriptObj != 0 && scriptFunc != 0)
		{
			// Prepare Function for execution
			mScriptContext->Prepare(scriptFunc);

			// Set Object pointer
			mScriptContext->SetObject(scriptObj);

			return true;
		}

		cout << "Either the Script Object or Function was null" << endl;
		return false;
	}

	bool AngelScriptSubsystem::ExecuteScriptMethod(void* scriptObj, asIScriptFunction* scriptFunc)
	{
		if (scriptObj != 0 && scriptFunc != 0)
		{
			// Execute the function
			int r = mScriptContext->Execute();
			if (r != asEXECUTION_FINISHED)
			{
				// The execution didn't finish as we had planned. Determine why.
				if (r == asEXECUTION_ABORTED)
					cout << "The script was aborted before it could finish. Probably it timed out." << endl;
				else if (r == asEXECUTION_EXCEPTION)
				{
					cout << "The script ended with an exception." << endl;

					// Write some information about the script exception
					asIScriptFunction* func = mScriptContext->GetExceptionFunction();
					cout << "func: " << func->GetDeclaration() << endl;
					cout << "modl: " << func->GetModuleName() << endl;
					//cout << "sect: " << func->GetScriptSectionName() << endl;
					cout << "line: " << mScriptContext->GetExceptionLineNumber() << endl;
					cout << "desc: " << mScriptContext->GetExceptionString() << endl;
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

	bool AngelScriptSubsystem::PrepareAndExecuteScriptMethod(void* script_obj, asIScriptFunction* script_func)
	{
		if (PrepareScriptMethod(script_obj, script_func))
		{
			return ExecuteScriptMethod(script_obj, script_func);
		}

		return false;
	}

	void AngelScriptSubsystem::SetCurrentEntityID(UUID id)
	{
		mCurrentEntityID = id;
	}

	// Global Script Functions

	const double& AngelScriptSubsystem::GetDeltaTime() const
	{
		return mEngine->GetDeltaTime();
	}

	const double& AngelScriptSubsystem::GetFixedTime() const
	{
		return mEngine->GetTimeStepFixed();
	}

	void AngelScriptSubsystem::PlaySoundEffect(uint64_t id, float volume, bool looping, bool restart)
	{
		if (const auto audioSubsystem = mEngine->GetSubsystem<audio::AudioSubsystem>())
		{
			//mAudioSubsystem->playSoundEffect(id, volume, looping, restart);
		}
	}

	UUID AngelScriptSubsystem::PlaySoundEffect(const std::string& path, float volume, bool looping, bool restart)
	{
		UUID id = 0;

		if (const auto audioSubsystem = mEngine->GetSubsystem<audio::AudioSubsystem>())
		{
			//id = mAudioSubsystem->playSoundEffect(path, volume, looping, restart);
		}

		return id;
	}

	UUID AngelScriptSubsystem::GetEntityID()
	{
		return mCurrentEntityID;
	}

	ScriptCallback AngelScriptSubsystem::BindCallback(UUID entity, asIScriptFunction* cb) const
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
			mScriptEngine->AddRefScriptObject(scriptCallback.object, scriptCallback.objectType);
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

	void AngelScriptSubsystem::ReleaseCallback(ScriptCallback& scriptCallback) const
	{
		if (scriptCallback.func)
			scriptCallback.func->Release();

		if (scriptCallback.object)
			mScriptEngine->ReleaseScriptObject(scriptCallback.object, scriptCallback.objectType);

		scriptCallback.func = nullptr;
		scriptCallback.object = nullptr;
		scriptCallback.objectType = nullptr;
	}

	// Collision Callbacks

	void AngelScriptSubsystem::BindOnCollisionBegin(UUID entity, asIScriptFunction* cb)
	{
		ReleaseOnCollisionBegin(entity);

		mOnCollisionBeginCallbacks[entity] = BindCallback(entity, cb);
	}

	void AngelScriptSubsystem::BindOnCollisionEnd(UUID entity, asIScriptFunction* cb)
	{
		ReleaseOnCollisionEnd(entity);

		mOnCollisionEndCallbacks[entity] = BindCallback(entity, cb);
	}

	void AngelScriptSubsystem::ReleaseOnCollisionBegin(UUID entity)
	{
		if (mOnCollisionBeginCallbacks.count(entity) == 1)
		{
			ReleaseCallback(mOnCollisionBeginCallbacks[entity]);
			mOnCollisionBeginCallbacks.erase(entity);
		}
	}

	void AngelScriptSubsystem::ReleaseOnCollisionEnd(UUID entity)
	{
		if (mOnCollisionEndCallbacks.count(entity) == 1)
		{
			ReleaseCallback(mOnCollisionEndCallbacks[entity]);
			mOnCollisionEndCallbacks.erase(entity);
		}
	}

	AngelScriptGameplaySubsystem::AngelScriptGameplaySubsystem(std::shared_ptr<core::Engine> engine) : Subsystem(engine)
	{
		mName = "AngelScriptGameplaySubsystem";
	}

	core::SubsystemType AngelScriptGameplaySubsystem::GetType() const
	{
		return core::SubsystemType::Gameplay;
	}

	void AngelScriptGameplaySubsystem::BeginPlay()
	{
		const auto angelscriptSubsystem = mEngine->GetSubsystem<AngelScriptSubsystem>();
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		// Execute update method on scripts
		const auto scriptView = registry->view<AngelScriptComponent>();

		for (auto [entity, script] : scriptView.each())
		{
			angelscriptSubsystem->SetCurrentEntityID(enttSubsystem->GetID(entity));

			angelscriptSubsystem->PrepareAndExecuteScriptMethod(script.obj, script.beginPlayFunc);
		}
	}

	void AngelScriptGameplaySubsystem::EndPlay()
	{
		const auto angelscriptSubsystem = mEngine->GetSubsystem<AngelScriptSubsystem>();
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		// Execute update method on scripts
		const auto scriptView = registry->view<AngelScriptComponent>();

		for (auto [entity, script] : scriptView.each())
		{
			angelscriptSubsystem->SetCurrentEntityID(enttSubsystem->GetID(entity));

			angelscriptSubsystem->PrepareAndExecuteScriptMethod(script.obj, script.endPlayFunc);
		}
	}

	void AngelScriptGameplaySubsystem::Update(double deltaTime)
	{
		const auto angelscriptSubsystem = mEngine->GetSubsystem<AngelScriptSubsystem>();
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		// Execute update method on scripts
		const auto scriptView = registry->view<AngelScriptComponent>();

		for (auto [entity, script] : scriptView.each())
		{
			angelscriptSubsystem->SetCurrentEntityID(enttSubsystem->GetID(entity));

			angelscriptSubsystem->PrepareAndExecuteScriptMethod(script.obj, script.updateFunc);
		}
	}

	bool AngelScriptGameplaySubsystem::ShouldUpdate()
	{
		return true;
	}

	void AngelScriptGameplaySubsystem::FixedUpdate(double fixedTime)
	{
		const auto angelscriptSubsystem = mEngine->GetSubsystem<AngelScriptSubsystem>();
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		// Execute fixed update method on scripts
		const auto scriptView = registry->view<AngelScriptComponent>();

		for (auto [entity, script] : scriptView.each())
		{
			angelscriptSubsystem->SetCurrentEntityID(enttSubsystem->GetID(entity));

			angelscriptSubsystem->PrepareAndExecuteScriptMethod(script.obj, script.fixedUpdateFunc);
		}
	}

	bool AngelScriptGameplaySubsystem::ShouldFixedUpdate()
	{
		return true;
	}

	
}
