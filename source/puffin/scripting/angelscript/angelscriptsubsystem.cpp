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
		m_engine_interface = nullptr;

		// Shut down the engine
		m_script_engine->ShutDownAndRelease();
		m_script_engine = nullptr;

		mEngine = nullptr;
	}

	void AngelScriptSubsystem::Initialize(core::SubsystemManager* subsystem_manager)
	{
		auto entt_subsystem = subsystem_manager->CreateAndInitializeSubsystem<ecs::EnTTSubsystem>();
		const auto registry = entt_subsystem->registry();

		registry->on_construct<AngelScriptComponent>().connect<&AngelScriptSubsystem::on_construct_script>(this);
		//registry->on_update<AngelScriptComponent>().connect<&AngelScriptSystem::onConstructScript>(this);
		registry->on_destroy<AngelScriptComponent>().connect<&AngelScriptSubsystem::on_destroy_script>(this);

		// Create Script Engine
		m_script_engine = asCreateScriptEngine();
		if (m_script_engine == nullptr)
		{
			cout << "Failed to create script engine." << endl;
		}

		// Set message callback to receive information on errors in human readable form
		const int r = m_script_engine->SetMessageCallback(asFUNCTION(messageCallback), nullptr, asCALL_CDECL); assert(r >= 0 && "Failed to set message callback for angelscript");

		// Configure Engine and Setup Global Function Callbacks
		configure_engine();

		m_engine_interface = std::make_unique<AngelScriptEngineInterface>(mEngine, m_script_engine);

		//mAudioSubsystem = mEngine->getSystem<audio::AudioSubsystem>();

		init_context();
		init_scripts();
	}

	void AngelScriptSubsystem::Deinitialize()
	{
		
	}

	void AngelScriptSubsystem::EndPlay()
	{
		// Execute Script Stop Methods
		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = entt_subsystem->registry();

		const auto scriptView = registry->view<AngelScriptComponent>();

		for (auto [entity, script] : scriptView.each())
		{
			m_current_entity_id = entt_subsystem->get_id(entity);

			destroy_script(script);
		}

		// Release Input JustPressed Callbacks
		for (auto& [string, callbacks] : m_on_input_pressed_callbacks)
		{
			for (auto& [id, callback] : callbacks)
			{
				release_callback(callback);
			}
		}

		m_on_input_pressed_callbacks.clear();

		// Release Input JustReleased Callbacks
		for (auto& [string, callbacks] : m_on_input_released_callbacks)
		{
			for (auto& [id, callback] : callbacks)
			{
				release_callback(callback);
			}
		}

		m_on_input_pressed_callbacks.clear();

		// Release collision begin callbacks
		for (auto& [id, callback] : m_on_collision_begin_callbacks)
		{
			release_callback(callback);
		}

		m_on_collision_begin_callbacks.clear();

		// Release collision end callbacks
		for (auto& [id, callback] : m_on_collision_end_callbacks)
		{
			release_callback(callback);
		}

		m_on_collision_end_callbacks.clear();

		// We must release the contexts when no longer using them
		if (m_script_context)
		{
			m_script_context->Release();
		}

		init_context();
		init_scripts();
	}

	void AngelScriptSubsystem::Update(double delta_time)
	{
		// Process Input Events
		process_events();

		// Destroy old scripts
		stop_scripts();

		// Initialize new scripts
		init_scripts();

		// Run new scripts start method
		start_scripts();
	}

	bool AngelScriptSubsystem::ShouldUpdate()
	{
		return true;
	}

	void AngelScriptSubsystem::on_construct_script(entt::registry& registry, entt::entity entity)
	{
		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto& id = entt_subsystem->get_id(entity);

		m_scripts_to_init.emplace(id);
	}

	void AngelScriptSubsystem::on_destroy_script(entt::registry& registry, entt::entity entity)
	{
		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto& id = entt_subsystem->get_id(entity);

		m_scripts_to_end_play.emplace(id);
	}

	void AngelScriptSubsystem::configure_engine()
	{
		int r;

		// Register the script string type
		// Look at the implementation for this function for more information  
		// on how to register a custom string type, and other object types.
		RegisterStdString(m_script_engine);

		if (!strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY"))
		{
			// Register the functions that the scripts will be allowed to use.
			// Note how the return code is validated with an assert(). This helps
			// us discover where a problem occurs, and doesn't pollute the code
			// with a lot of if's. If an error occurs in release mode it will
			// be caught when a script is being built, so it is not necessary
			// to do the verification here as well.
			r = m_script_engine->RegisterGlobalFunction("void Print(string &in)", asFUNCTION(printString), asCALL_CDECL); assert(r >= 0);
		}
		else
		{
			// Notice how the registration is almost identical to the above. 
			r = m_script_engine->RegisterGlobalFunction("void Print(string &in)", asFUNCTION(printStringGeneric), asCALL_GENERIC); assert(r >= 0);
		}

		// Define Global Methods for Scripts
		r = m_script_engine->RegisterGlobalFunction("uint64 GetEntityID()", asMETHOD(AngelScriptSubsystem, get_entity_id), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);

		r = m_script_engine->RegisterGlobalFunction("void PlaySoundEffect(uint64, float, bool, bool)", asMETHODPR(AngelScriptSubsystem, play_sound_effect, (uint64_t, float, bool, bool), void), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = m_script_engine->RegisterGlobalFunction("uint64 PlaySoundEffect(const string &in, float, bool, bool)", asMETHODPR(AngelScriptSubsystem, play_sound_effect, (const string&, float, bool, bool), uint64_t), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);

		// Register Components and their constructors, functions and properties
		RegisterTransformComponent(m_script_engine);

		// Register Collision Funcdefs and Bind/Release Callback Methods
		r = m_script_engine->RegisterFuncdef("void OnCollisionBeginCallback(uint64)"); assert(r >= 0);
		r = m_script_engine->RegisterFuncdef("void OnCollisionEndCallback(uint64)"); assert(r >= 0);

		r = m_script_engine->RegisterGlobalFunction("void BindOnCollisionBegin(uint64, OnCollisionBeginCallback @cb)", asMETHOD(AngelScriptSubsystem, bind_on_collision_begin), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = m_script_engine->RegisterGlobalFunction("void BindOnCollisionEnd(uint64, OnCollisionEndCallback @cb)", asMETHOD(AngelScriptSubsystem, bind_on_collision_end), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = m_script_engine->RegisterGlobalFunction("void ReleaseOnCollisionBegin(uint64)", asMETHOD(AngelScriptSubsystem, release_on_collision_begin), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = m_script_engine->RegisterGlobalFunction("void ReleaseOnCollisionEnd(uint64)", asMETHOD(AngelScriptSubsystem, release_on_collision_end), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);

		// It is possible to register the functions, properties, and types in 
		// configuration groups as well. When compiling the scripts it then
		// be defined which configuration groups should be available for that
		// script. If necessary a configuration group can also be removed from
		// the engine, so that the engine configuration could be changed 
		// without having to recompile all the scripts.
	}

	void AngelScriptSubsystem::init_context()
	{
		m_script_context = m_script_engine->CreateContext();

		if (m_script_context == nullptr)
		{
			cout << "Failed to create the context." << endl;
			m_script_engine->Release();
		}
	}

	void AngelScriptSubsystem::init_scripts()
	{
		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = entt_subsystem->registry();

		for (const auto id : m_scripts_to_init)
		{
			entt::entity entity = entt_subsystem->get_entity(id);

			auto& script = registry->get<AngelScriptComponent>(entity);

			initialize_script(id, script);

			ExportEditablePropertiesToScriptData(script, script.serializedData);

			m_scripts_to_begin_play.emplace(id);
		}

		m_scripts_to_init.clear();
	}

	void AngelScriptSubsystem::start_scripts()
	{
		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = entt_subsystem->registry();

		for (const auto id : m_scripts_to_begin_play)
		{
			entt::entity entity = entt_subsystem->get_entity(id);

			const auto& script = registry->get<AngelScriptComponent>(entity);

			m_current_entity_id = id;

			prepare_and_execute_script_method(script.obj, script.beginPlayFunc);
		}

		m_scripts_to_begin_play.clear();
	}

	void AngelScriptSubsystem::stop_scripts()
	{
		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = entt_subsystem->registry();

		for (const auto id : m_scripts_to_end_play)
		{
			entt::entity entity = entt_subsystem->get_entity(id);

			auto& script = registry->get<AngelScriptComponent>(entity);

			m_current_entity_id = id;

			prepare_and_execute_script_method(script.obj, script.endPlayFunc);

			destroy_script(script);
		}

		m_scripts_to_end_play.clear();
	}


	void AngelScriptSubsystem::initialize_script(UUID entity, AngelScriptComponent& script)
	{
		compile_script(script);
		update_script_methods(script);
		instantiate_script_obj(entity, script);
		ImportEditableProperties(script, script.serializedData);
	}

	void AngelScriptSubsystem::compile_script(AngelScriptComponent& script) const
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
		r = builder.StartNewModule(m_script_engine, script.name.c_str());
		if (r < 0)
		{
			cout << "Failed to start new module" << endl;
		}

		fs::path scriptPath = assets::AssetRegistry::Get()->GetContentRoot() / script.dir;

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
			asIScriptModule* mod = m_script_engine->GetModule(script.name.c_str());

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

	void AngelScriptSubsystem::update_script_methods(AngelScriptComponent& script)
	{
		if (script.type != nullptr)
		{
			script.beginPlayFunc = get_script_method(script, "BeginPlay");
			script.fixedUpdateFunc = get_script_method(script, "FixedUpdate");
			script.updateFunc = get_script_method(script, "Update");
			script.endPlayFunc = get_script_method(script, "EndPlay");
		}
	}

	void AngelScriptSubsystem::instantiate_script_obj(UUID entity, AngelScriptComponent& script)
	{
		if (script.type != 0 && script.type->GetFactoryCount() > 0)
		{
			// Create the type using its factory function
			asIScriptFunction* factory = script.type->GetFactoryByIndex(0);

			// Prepare context to call factory function
			m_script_context->Prepare(factory);

			// Execute Call
			m_script_context->Execute();

			// Get created object if nullptr, and increase its reference count by one
			script.obj = *(asIScriptObject**)m_script_context->GetAddressOfReturnValue();
			script.obj->AddRef();
		}
	}

	asIScriptFunction* AngelScriptSubsystem::get_script_method(const AngelScriptComponent& script, const char* funcName)
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

	void AngelScriptSubsystem::destroy_script(AngelScriptComponent& script)
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

	void AngelScriptSubsystem::process_events()
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

	bool AngelScriptSubsystem::prepare_script_method(void* scriptObj, asIScriptFunction* scriptFunc)
	{
		if (scriptObj != 0 && scriptFunc != 0)
		{
			// Prepare Function for execution
			m_script_context->Prepare(scriptFunc);

			// Set Object pointer
			m_script_context->SetObject(scriptObj);

			return true;
		}

		cout << "Either the Script Object or Function was null" << endl;
		return false;
	}

	bool AngelScriptSubsystem::execute_script_method(void* scriptObj, asIScriptFunction* scriptFunc)
	{
		if (scriptObj != 0 && scriptFunc != 0)
		{
			// Execute the function
			int r = m_script_context->Execute();
			if (r != asEXECUTION_FINISHED)
			{
				// The execution didn't finish as we had planned. Determine why.
				if (r == asEXECUTION_ABORTED)
					cout << "The script was aborted before it could finish. Probably it timed out." << endl;
				else if (r == asEXECUTION_EXCEPTION)
				{
					cout << "The script ended with an exception." << endl;

					// Write some information about the script exception
					asIScriptFunction* func = m_script_context->GetExceptionFunction();
					cout << "func: " << func->GetDeclaration() << endl;
					cout << "modl: " << func->GetModuleName() << endl;
					//cout << "sect: " << func->GetScriptSectionName() << endl;
					cout << "line: " << m_script_context->GetExceptionLineNumber() << endl;
					cout << "desc: " << m_script_context->GetExceptionString() << endl;
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

	bool AngelScriptSubsystem::prepare_and_execute_script_method(void* script_obj, asIScriptFunction* script_func)
	{
		if (prepare_script_method(script_obj, script_func))
		{
			return execute_script_method(script_obj, script_func);
		}

		return false;
	}

	void AngelScriptSubsystem::set_current_entity_id(UUID id)
	{
		m_current_entity_id = id;
	}

	// Global Script Functions

	const double& AngelScriptSubsystem::get_delta_time() const
	{
		const double deltaTime = mEngine->GetDeltaTime();
		return deltaTime;
	}

	const double& AngelScriptSubsystem::get_fixed_time() const
	{
		const double fixedDeltaTime = mEngine->GetTimeStepFixed();
		return fixedDeltaTime;
	}

	void AngelScriptSubsystem::play_sound_effect(uint64_t id, float volume, bool looping, bool restart)
	{
		auto audio_subsystem = mEngine->GetSubsystem<audio::AudioSubsystem>();
		if (audio_subsystem)
		{
			//mAudioSubsystem->playSoundEffect(id, volume, looping, restart);
		}
	}

	UUID AngelScriptSubsystem::play_sound_effect(const std::string& path, float volume, bool looping, bool restart)
	{
		UUID id = 0;

		auto audio_subsystem = mEngine->GetSubsystem<audio::AudioSubsystem>();
		if (audio_subsystem)
		{
			//id = mAudioSubsystem->playSoundEffect(path, volume, looping, restart);
		}

		return id;
	}

	UUID AngelScriptSubsystem::get_entity_id()
	{
		return m_current_entity_id;
	}

	ScriptCallback AngelScriptSubsystem::bind_callback(UUID entity, asIScriptFunction* cb) const
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
			m_script_engine->AddRefScriptObject(scriptCallback.object, scriptCallback.objectType);
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

	void AngelScriptSubsystem::release_callback(ScriptCallback& scriptCallback) const
	{
		if (scriptCallback.func)
			scriptCallback.func->Release();

		if (scriptCallback.object)
			m_script_engine->ReleaseScriptObject(scriptCallback.object, scriptCallback.objectType);

		scriptCallback.func = nullptr;
		scriptCallback.object = nullptr;
		scriptCallback.objectType = nullptr;
	}

	// Collision Callbacks

	void AngelScriptSubsystem::bind_on_collision_begin(UUID entity, asIScriptFunction* cb)
	{
		release_on_collision_begin(entity);

		m_on_collision_begin_callbacks[entity] = bind_callback(entity, cb);
	}

	void AngelScriptSubsystem::bind_on_collision_end(UUID entity, asIScriptFunction* cb)
	{
		release_on_collision_end(entity);

		m_on_collision_end_callbacks[entity] = bind_callback(entity, cb);
	}

	void AngelScriptSubsystem::release_on_collision_begin(UUID entity)
	{
		if (m_on_collision_begin_callbacks.count(entity) == 1)
		{
			release_callback(m_on_collision_begin_callbacks[entity]);
			m_on_collision_begin_callbacks.erase(entity);
		}
	}

	void AngelScriptSubsystem::release_on_collision_end(UUID entity)
	{
		if (m_on_collision_end_callbacks.count(entity) == 1)
		{
			release_callback(m_on_collision_end_callbacks[entity]);
			m_on_collision_end_callbacks.erase(entity);
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
		auto angelscript_subsystem = mEngine->GetSubsystem<AngelScriptSubsystem>();
		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = entt_subsystem->registry();

		// Execute update method on scripts
		const auto script_view = registry->view<AngelScriptComponent>();

		for (auto [entity, script] : script_view.each())
		{
			angelscript_subsystem->set_current_entity_id(entt_subsystem->get_id(entity));

			angelscript_subsystem->prepare_and_execute_script_method(script.obj, script.beginPlayFunc);
		}
	}

	void AngelScriptGameplaySubsystem::EndPlay()
	{
		auto angelscript_subsystem = mEngine->GetSubsystem<AngelScriptSubsystem>();
		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = entt_subsystem->registry();

		// Execute update method on scripts
		const auto script_view = registry->view<AngelScriptComponent>();

		for (auto [entity, script] : script_view.each())
		{
			angelscript_subsystem->set_current_entity_id(entt_subsystem->get_id(entity));

			angelscript_subsystem->prepare_and_execute_script_method(script.obj, script.endPlayFunc);
		}
	}

	void AngelScriptGameplaySubsystem::Update(double delta_time)
	{
		auto angelscript_subsystem = mEngine->GetSubsystem<AngelScriptSubsystem>();
		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = entt_subsystem->registry();

		// Execute update method on scripts
		const auto script_view = registry->view<AngelScriptComponent>();

		for (auto [entity, script] : script_view.each())
		{
			angelscript_subsystem->set_current_entity_id(entt_subsystem->get_id(entity));

			angelscript_subsystem->prepare_and_execute_script_method(script.obj, script.updateFunc);
		}
	}

	bool AngelScriptGameplaySubsystem::ShouldUpdate()
	{
		return true;
	}

	void AngelScriptGameplaySubsystem::FixedUpdate(double fixed_time)
	{
		auto angelscript_subsystem = mEngine->GetSubsystem<AngelScriptSubsystem>();
		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = entt_subsystem->registry();

		// Execute fixed update method on scripts
		const auto script_view = registry->view<AngelScriptComponent>();

		for (auto [entity, script] : script_view.each())
		{
			angelscript_subsystem->set_current_entity_id(entt_subsystem->get_id(entity));

			angelscript_subsystem->prepare_and_execute_script_method(script.obj, script.fixedUpdateFunc);
		}
	}

	bool AngelScriptGameplaySubsystem::ShouldFixedUpdate()
	{
		return true;
	}

	
}
