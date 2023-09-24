#pragma once

// AngelScript Includes
#include "angelscript/angelscript.h"
#include "angelscript/scriptbuilder/scriptbuilder.h"
#include "angelscript/scriptstdstring/scriptstdstring.h"

#include "Core/System.h"
#include "Audio/AudioSubsystem.h"
#include "Components/Scripting/AngelScriptComponent.h"
#include "Core/Engine.h"
#include "Input/InputEvent.h"
#include "Physics/CollisionEvent.h"
#include "Types/RingBuffer.h"
#include "Types/UUID.h"

#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "ECS/EnTTSubsystem.h"

namespace puffin::scripting
{
	struct ScriptCallback
	{
		PuffinID entity;
		asIScriptFunction* func = nullptr;
		void* object = nullptr;
		asITypeInfo* objectType = nullptr;
	};

	typedef std::map<PuffinID, ScriptCallback> ScriptCallbackMap;

	class AngelScriptSystem : public core::System
	{
	public:

		AngelScriptSystem();
		~AngelScriptSystem() override;

		void setupCallbacks() override
		{
			mEngine->registerCallback(core::ExecutionStage::Init, [&] { init(); }, "AngelScriptSystem: Init", 250);
			mEngine->registerCallback(core::ExecutionStage::BeginPlay, [&] { beginPlay(); }, "AngelScriptSystem: BeginPlay");
			mEngine->registerCallback(core::ExecutionStage::FixedUpdate, [&] { fixedUpdate(); }, "AngelScriptSystem: FixedUpdate");
			mEngine->registerCallback(core::ExecutionStage::Update, [&] { update(); }, "AngelScriptSystem: Update");
			mEngine->registerCallback(core::ExecutionStage::EndPlay, [&] { endPlay(); }, "AngelScriptSystem: EndPlay");

			const auto registry = mEngine->getSubsystem<ecs::EnTTSubsystem>()->registry();

			registry->on_construct<AngelScriptComponent>().connect<&AngelScriptSystem::onConstructScript>(this);
			//registry->on_update<AngelScriptComponent>().connect<&AngelScriptSystem::onConstructScript>(this);
			registry->on_destroy<AngelScriptComponent>().connect<&AngelScriptSystem::onDestroyScript>(this);
		}

		void init();
		void beginPlay();
		void fixedUpdate();
		void update();
		void endPlay();

		void onConstructScript(entt::registry& registry, entt::entity entity);
		void onDestroyScript(entt::registry& registry, entt::entity entity);

		// Hot-Reloads all scripts when called
		//void reload() {}

	private:

		asIScriptEngine* mScriptEngine = nullptr;
		asIScriptContext* mCtx = nullptr;

		std::shared_ptr<audio::AudioSubsystem> mAudioSubsystem;

		PuffinID mCurrentEntityID; // Entity ID for currently executing script

		// Event Buffers
		std::shared_ptr<RingBuffer<input::InputEvent>> mInputEvents = nullptr;;
		std::shared_ptr<RingBuffer<physics::CollisionBeginEvent>> mCollisionBeginEvents = nullptr;
		std::shared_ptr<RingBuffer<physics::CollisionEndEvent>> mCollisionEndEvents = nullptr;

		// Maps of Input Callbacks
		std::unordered_map<std::string, ScriptCallbackMap> mOnInputPressedCallbacks;
		std::unordered_map<std::string, ScriptCallbackMap> mOnInputReleasedCallbacks;

		// Collision Callbacks
		ScriptCallbackMap mOnCollisionBeginCallbacks;
		ScriptCallbackMap mOnCollisionEndCallbacks;

		std::unordered_set<PuffinID> mScriptsToInit;
		std::unordered_set<PuffinID> mScriptsToStart;
		std::unordered_set<PuffinID> mScriptsToStop;

		void configureEngine();

		void initContext();
		void initScripts();
		void startScripts();
		void stopScripts();

		void initializeScript(PuffinID entity, AngelScriptComponent& script);
		void compileScript(AngelScriptComponent& script) const;
		void updateScriptMethods(AngelScriptComponent& script);
		void instantiateScriptObj(PuffinID entity, AngelScriptComponent& script);

		void destroyScript(AngelScriptComponent& script);

		void processEvents();

		asIScriptFunction* getScriptMethod(const AngelScriptComponent& script, const char* funcName);
		bool prepareScriptMethod(void* scriptObj, asIScriptFunction* scriptFunc);
		bool executeScriptMethod(void* scriptObj, asIScriptFunction* scriptFunc);
		bool prepareAndExecuteScriptMethod(void* scriptObj, asIScriptFunction* scriptFunc);

		// Global Script Functions
		[[nodiscard]] const double& getDeltaTime() const;
		[[nodiscard]] const double& getFixedTime() const;

		void playSoundEffect(uint64_t id, float volume = 1.0f, bool looping = false, bool restart = false);
		PuffinID playSoundEffect(const std::string& path, float volume = 1.0f, bool looping = false,
		                         bool restart = false);

		PuffinID getEntityID(); // Return the Entity ID for the attached script

		// Script Callbacks
		ScriptCallback bindCallback(PuffinID entity, asIScriptFunction* cb) const;
		void releaseCallback(ScriptCallback& scriptCallback) const;

		// Input Functions
		void bindOnInputPressed(PuffinID entity, const std::string& actionName, asIScriptFunction* cb);
		void bindOnInputReleased(PuffinID entity, const std::string& actionName, asIScriptFunction *cb);

		void releaseOnInputPressed(PuffinID entity, const std::string& actionName);
		void releaseOnInputReleased(PuffinID entity, const std::string& actionName);

		// Collision Functions
		void bindOnCollisionBegin(PuffinID entity, asIScriptFunction* cb);
		void bindOnCollisionEnd(PuffinID entity, asIScriptFunction* cb);

		void releaseOnCollisionBegin(PuffinID entity);
		void releaseOnCollisionEnd(PuffinID entity);
	};
}
