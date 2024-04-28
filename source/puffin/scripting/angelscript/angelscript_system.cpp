#include "puffin/scripting/angelscript/angelscript_system.h"

#include <assert.h>  // assert()
#include <iostream>  // cout
#include <string.h>
#include <vector>

#include "puffin/core/engine.h"
#include "puffin/assets/asset_registry.h"
#include "angelscript/scriptbuilder/scriptbuilder.h"
#include "angelscript/scriptstdstring/scriptstdstring.h"
#include "entt/entity/registry.hpp"
#include "puffin/ecs/entt_subsystem.h"
#include "puffin/scripting/angelscript/angelscript_engine_interface.h"
#include "puffin/scripting/angelscript/register_type_helpers.h"

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
	AngelScriptSystem::AngelScriptSystem(const std::shared_ptr<core::Engine>& engine) : System(engine)
	{
		mEngine->registerCallback(core::ExecutionStage::Startup, [&] { startup(); }, "AngelScriptSystem: Startup", 250);
		mEngine->registerCallback(core::ExecutionStage::BeginPlay, [&] { beginPlay(); }, "AngelScriptSystem: BeginPlay");
		mEngine->registerCallback(core::ExecutionStage::FixedUpdate, [&] { fixedUpdate(); }, "AngelScriptSystem: FixedUpdate");
		mEngine->registerCallback(core::ExecutionStage::Update, [&] { update(); }, "AngelScriptSystem: Update");
		mEngine->registerCallback(core::ExecutionStage::EndPlay, [&] { endPlay(); }, "AngelScriptSystem: EndPlay");

		const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

		registry->on_construct<AngelScriptComponent>().connect<&AngelScriptSystem::onConstructScript>(this);
		//registry->on_update<AngelScriptComponent>().connect<&AngelScriptSystem::onConstructScript>(this);
		registry->on_destroy<AngelScriptComponent>().connect<&AngelScriptSystem::onDestroyScript>(this);

		// Create Script Engine
		mScriptEngine = asCreateScriptEngine();
		if (mScriptEngine == nullptr)
		{
			cout << "Failed to create script engine." << endl;
		}

		// Set message callback to receive information on errors in human readable form
		const int r = mScriptEngine->SetMessageCallback(asFUNCTION(messageCallback), nullptr, asCALL_CDECL); assert(r >= 0 && "Failed to set message callback for angelscript");
	}

	AngelScriptSystem::~AngelScriptSystem()
	{
		mEngineInterface = nullptr;

		// Shut down the engine
		mScriptEngine->ShutDownAndRelease();
		mScriptEngine = nullptr;

		mEngine = nullptr;
	}

	void AngelScriptSystem::startup()
	{
		// Configure Engine and Setup Global Function Callbacks
		configureEngine();

		mEngineInterface = std::make_unique<AngelScriptEngineInterface>(mEngine, shared_from_this(), mScriptEngine);

		//mAudioSubsystem = mEngine->getSystem<audio::AudioSubsystem>();

		initContext();
		initScripts();
	}

	void AngelScriptSystem::beginPlay()
	{
		// Execute Start Methods
		startScripts();
	}

	void AngelScriptSystem::fixedUpdate()
	{
		// Initialize/Cleanup marked components
		const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

		const auto scriptView = registry->view<AngelScriptComponent>();

		for (auto [entity, script] : scriptView.each())
		{
			mCurrentEntityID = mEngine->getSystem<ecs::EnTTSubsystem>()->get_id(entity);

			// Execute Update function if one was found for script
			prepareAndExecuteScriptMethod(script.obj, script.fixedUpdateFunc);
		}
	}

	void AngelScriptSystem::update()
	{
		// Process Input Events
		processEvents();

		// Destroy old scripts
		stopScripts();

		// Initialize new scripts
		initScripts();

		// Run new scripts start method
		startScripts();
		
		// Run all scripts update method
		{
			const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

			const auto scriptView = registry->view<AngelScriptComponent>();

			for (auto [entity, script] : scriptView.each())
			{
				mCurrentEntityID = mEngine->getSystem<ecs::EnTTSubsystem>()->get_id(entity);

				// Execute Update function if one was found for script
				prepareAndExecuteScriptMethod(script.obj, script.updateFunc);
			}
		}
	}

	void AngelScriptSystem::endPlay()
	{
		// Execute Script Stop Methods
		const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

		const auto scriptView = registry->view<AngelScriptComponent>();

		for (auto [entity, script] : scriptView.each())
		{
			mCurrentEntityID = mEngine->getSystem<ecs::EnTTSubsystem>()->get_id(entity);

			prepareAndExecuteScriptMethod(script.obj, script.stopFunc);

			destroyScript(script);
		}

		// Release Input JustPressed Callbacks
		for (auto& [string, callbacks] : mOnInputPressedCallbacks)
		{
			for (auto& [id, callback] : callbacks)
			{
				releaseCallback(callback);
			}
		}

		mOnInputPressedCallbacks.clear();

		// Release Input JustReleased Callbacks
		for (auto& [string, callbacks] : mOnInputReleasedCallbacks)
		{
			for (auto& [id, callback] : callbacks)
			{
				releaseCallback(callback);
			}
		}

		mOnInputPressedCallbacks.clear();

		// Release collision begin callbacks
		for (auto& [id, callback] : mOnCollisionBeginCallbacks)
		{
			releaseCallback(callback);
		}

		mOnCollisionBeginCallbacks.clear();

		// Release collision end callbacks
		for (auto& [id, callback] : mOnCollisionEndCallbacks)
		{
			releaseCallback(callback);
		}

		mOnCollisionEndCallbacks.clear();

		// We must release the contexts when no longer using them
		if (mCtx)
		{
			mCtx->Release();
		}

		initContext();
		initScripts();
	}

	void AngelScriptSystem::onConstructScript(entt::registry& registry, entt::entity entity)
	{
		const auto& id = mEngine->getSystem<ecs::EnTTSubsystem>()->get_id(entity);

		mScriptsToInit.emplace(id);
	}

	void AngelScriptSystem::onDestroyScript(entt::registry& registry, entt::entity entity)
	{
		const auto& id = mEngine->getSystem<ecs::EnTTSubsystem>()->get_id(entity);

		mScriptsToStop.emplace(id);
	}

	void AngelScriptSystem::configureEngine()
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
		r = mScriptEngine->RegisterGlobalFunction("uint64 GetEntityID()", asMETHOD(AngelScriptSystem, getEntityID), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);

		r = mScriptEngine->RegisterGlobalFunction("void PlaySoundEffect(uint64, float, bool, bool)", asMETHODPR(AngelScriptSystem, playSoundEffect, (uint64_t, float, bool, bool), void), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = mScriptEngine->RegisterGlobalFunction("uint64 PlaySoundEffect(const string &in, float, bool, bool)", asMETHODPR(AngelScriptSystem, playSoundEffect, (const string&, float, bool, bool), uint64_t), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);

		// Register Components and their constructors, functions and properties
		RegisterTransformComponent(mScriptEngine);

		// Register Collision Funcdefs and Bind/Release Callback Methods
		r = mScriptEngine->RegisterFuncdef("void OnCollisionBeginCallback(uint64)"); assert(r >= 0);
		r = mScriptEngine->RegisterFuncdef("void OnCollisionEndCallback(uint64)"); assert(r >= 0);

		r = mScriptEngine->RegisterGlobalFunction("void BindOnCollisionBegin(uint64, OnCollisionBeginCallback @cb)", asMETHOD(AngelScriptSystem, bindOnCollisionBegin), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = mScriptEngine->RegisterGlobalFunction("void BindOnCollisionEnd(uint64, OnCollisionEndCallback @cb)", asMETHOD(AngelScriptSystem, bindOnCollisionEnd), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = mScriptEngine->RegisterGlobalFunction("void ReleaseOnCollisionBegin(uint64)", asMETHOD(AngelScriptSystem, releaseOnCollisionBegin), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = mScriptEngine->RegisterGlobalFunction("void ReleaseOnCollisionEnd(uint64)", asMETHOD(AngelScriptSystem, releaseOnCollisionEnd), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);

		// It is possible to register the functions, properties, and types in 
		// configuration groups as well. When compiling the scripts it then
		// be defined which configuration groups should be available for that
		// script. If necessary a configuration group can also be removed from
		// the engine, so that the engine configuration could be changed 
		// without having to recompile all the scripts.
	}

	void AngelScriptSystem::initContext()
	{
		mCtx = mScriptEngine->CreateContext();

		if (mCtx == nullptr)
		{
			cout << "Failed to create the context." << endl;
			mScriptEngine->Release();
		}
	}

	void AngelScriptSystem::initScripts()
	{
		const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

		for (const auto id : mScriptsToInit)
		{
			entt::entity entity = mEngine->getSystem<ecs::EnTTSubsystem>()->get_entity(id);

			auto& script = registry->get<AngelScriptComponent>(entity);

			initializeScript(id, script);

			ExportEditablePropertiesToScriptData(script, script.serializedData);

			mScriptsToStart.emplace(id);
		}

		mScriptsToInit.clear();
	}

	void AngelScriptSystem::startScripts()
	{
		const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

		for (const auto id : mScriptsToStart)
		{
			entt::entity entity = mEngine->getSystem<ecs::EnTTSubsystem>()->get_entity(id);

			const auto& script = registry->get<AngelScriptComponent>(entity);

			mCurrentEntityID = id;

			prepareAndExecuteScriptMethod(script.obj, script.startFunc);
		}

		mScriptsToStart.clear();
	}

	void AngelScriptSystem::stopScripts()
	{
		const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

		for (const auto id : mScriptsToStop)
		{
			entt::entity entity = mEngine->getSystem<ecs::EnTTSubsystem>()->get_entity(id);

			auto& script = registry->get<AngelScriptComponent>(entity);

			mCurrentEntityID = id;

			prepareAndExecuteScriptMethod(script.obj, script.stopFunc);

			destroyScript(script);
		}

		mScriptsToStop.clear();
	}


	void AngelScriptSystem::initializeScript(PuffinID entity, AngelScriptComponent& script)
	{
		compileScript(script);
		updateScriptMethods(script);
		instantiateScriptObj(entity, script);
		ImportEditableProperties(script, script.serializedData);
	}

	void AngelScriptSystem::compileScript(AngelScriptComponent& script) const
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

		fs::path scriptPath = assets::AssetRegistry::get()->contentRoot() / script.dir;

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

	void AngelScriptSystem::updateScriptMethods(AngelScriptComponent& script)
	{
		if (script.type != nullptr)
		{
			script.startFunc = getScriptMethod(script, "Start");
			script.fixedUpdateFunc = getScriptMethod(script, "FixedUpdate");
			script.updateFunc = getScriptMethod(script, "Update");
			script.stopFunc = getScriptMethod(script, "Stop");
		}
	}

	void AngelScriptSystem::instantiateScriptObj(PuffinID entity, AngelScriptComponent& script)
	{
		if (script.type != 0 && script.type->GetFactoryCount() > 0)
		{
			// Create the type using its factory function
			asIScriptFunction* factory = script.type->GetFactoryByIndex(0);

			// Prepare context to call factory function
			mCtx->Prepare(factory);

			// Execute Call
			mCtx->Execute();

			// Get created object if nullptr, and increase its reference count by one
			script.obj = *(asIScriptObject**)mCtx->GetAddressOfReturnValue();
			script.obj->AddRef();
		}
	}

	asIScriptFunction* AngelScriptSystem::getScriptMethod(const AngelScriptComponent& script, const char* funcName)
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

	void AngelScriptSystem::destroyScript(AngelScriptComponent& script)
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

	void AngelScriptSystem::processEvents()
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

	bool AngelScriptSystem::prepareScriptMethod(void* scriptObj, asIScriptFunction* scriptFunc)
	{
		if (scriptObj != 0 && scriptFunc != 0)
		{
			// Prepare Function for execution
			mCtx->Prepare(scriptFunc);

			// Set Object pointer
			mCtx->SetObject(scriptObj);

			return true;
		}

		cout << "Either the Script Object or Function was null" << endl;
		return false;
	}

	bool AngelScriptSystem::executeScriptMethod(void* scriptObj, asIScriptFunction* scriptFunc)
	{
		if (scriptObj != 0 && scriptFunc != 0)
		{
			// Execute the function
			int r = mCtx->Execute();
			if (r != asEXECUTION_FINISHED)
			{
				// The execution didn't finish as we had planned. Determine why.
				if (r == asEXECUTION_ABORTED)
					cout << "The script was aborted before it could finish. Probably it timed out." << endl;
				else if (r == asEXECUTION_EXCEPTION)
				{
					cout << "The script ended with an exception." << endl;

					// Write some information about the script exception
					asIScriptFunction* func = mCtx->GetExceptionFunction();
					cout << "func: " << func->GetDeclaration() << endl;
					cout << "modl: " << func->GetModuleName() << endl;
					//cout << "sect: " << func->GetScriptSectionName() << endl;
					cout << "line: " << mCtx->GetExceptionLineNumber() << endl;
					cout << "desc: " << mCtx->GetExceptionString() << endl;
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

	bool AngelScriptSystem::prepareAndExecuteScriptMethod(void* scriptObj, asIScriptFunction* scriptFunc)
	{
		if (prepareScriptMethod(scriptObj, scriptFunc))
		{
			return executeScriptMethod(scriptObj, scriptFunc);
		}

		return false;
	}

	// Global Script Functions

	const double& AngelScriptSystem::getDeltaTime() const
	{
		const double deltaTime = mEngine->deltaTime();
		return deltaTime;
	}

	const double& AngelScriptSystem::getFixedTime() const
	{
		const double fixedDeltaTime = mEngine->timeStepFixed();
		return fixedDeltaTime;
	}

	void AngelScriptSystem::playSoundEffect(uint64_t id, float volume, bool looping, bool restart)
	{
		if (mAudioSubsystem)
		{
			//mAudioSubsystem->playSoundEffect(id, volume, looping, restart);
		}
	}

	PuffinID AngelScriptSystem::playSoundEffect(const std::string& path, float volume, bool looping, bool restart)
	{
		PuffinID id = 0;

		if (mAudioSubsystem)
		{
			//id = mAudioSubsystem->playSoundEffect(path, volume, looping, restart);
		}

		return id;
	}

	PuffinID AngelScriptSystem::getEntityID()
	{
		return mCurrentEntityID;
	}

	ScriptCallback AngelScriptSystem::bindCallback(PuffinID entity, asIScriptFunction* cb) const
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

	void AngelScriptSystem::releaseCallback(ScriptCallback& scriptCallback) const
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

	void AngelScriptSystem::bindOnCollisionBegin(PuffinID entity, asIScriptFunction* cb)
	{
		releaseOnCollisionBegin(entity);

		mOnCollisionBeginCallbacks[entity] = bindCallback(entity, cb);
	}

	void AngelScriptSystem::bindOnCollisionEnd(PuffinID entity, asIScriptFunction* cb)
	{
		releaseOnCollisionEnd(entity);

		mOnCollisionEndCallbacks[entity] = bindCallback(entity, cb);
	}

	void AngelScriptSystem::releaseOnCollisionBegin(PuffinID entity)
	{
		if (mOnCollisionBeginCallbacks.count(entity) == 1)
		{
			releaseCallback(mOnCollisionBeginCallbacks[entity]);
			mOnCollisionBeginCallbacks.erase(entity);
		}
	}

	void AngelScriptSystem::releaseOnCollisionEnd(PuffinID entity)
	{
		if (mOnCollisionEndCallbacks.count(entity) == 1)
		{
			releaseCallback(mOnCollisionEndCallbacks[entity]);
			mOnCollisionEndCallbacks.erase(entity);
		}
	}
}