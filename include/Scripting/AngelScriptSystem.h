#pragma once

// AngelScript Includes
#define ANGELSCRIPT_DLL_LIBRARY_IMPORT
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
			mEngine->registerCallback(core::ExecutionStage::Init, [&]() { init(); }, "AngelScriptSystem: Init");
			mEngine->registerCallback(core::ExecutionStage::Setup, [&]() { setup(); }, "AngelScriptSystem: Setup");
			mEngine->registerCallback(core::ExecutionStage::Start, [&]() { start(); }, "AngelScriptSystem: Start");
			mEngine->registerCallback(core::ExecutionStage::Update, [&]() { update(); }, "AngelScriptSystem: Update");
			mEngine->registerCallback(core::ExecutionStage::Stop, [&]() { stop(); }, "AngelScriptSystem: Stop");
		}

		void init();
		void setup();
		void start();
		void update();
		void stop();

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

		bool mFirstInitialize = true;

		void configureEngine();

		void initializeScript(PuffinID entity, AngelScriptComponent& script);
		void compileScript(AngelScriptComponent& script);
		void updateScriptMethods(AngelScriptComponent& script);
		void instantiateScriptObj(PuffinID entity, AngelScriptComponent& script);

		void cleanupScriptComponent(AngelScriptComponent& script);

		void processEvents();

		asIScriptFunction* getScriptMethod(const AngelScriptComponent& script, const char* funcName);
		bool prepareScriptMethod(void* scriptObj, asIScriptFunction* scriptFunc);
		bool executeScriptMethod(void* scriptObj, asIScriptFunction* scriptFunc);
		bool prepareAndExecuteScriptMethod(void* scriptObj, asIScriptFunction* scriptFunc);

		// Global Script Functions
		[[nodiscard]] const double& getDeltaTime() const;
		[[nodiscard]] const double& getFixedTime() const;

		void playSoundEffect(uint64_t id, float volume = 1.0f, bool looping = false, bool restart = false);
		uint64_t playSoundEffect(const std::string& path, float volume = 1.0f, bool looping = false, bool restart = false);

		uint64_t getEntityID(); // Return the Entity ID for the attached script

		// Script Callbacks
		ScriptCallback bindCallback(uint32_t entity, asIScriptFunction* cb) const;
		void releaseCallback(ScriptCallback& scriptCallback) const;

		// Input Functions
		void bindOnInputPressed(uint32_t entity, const std::string& actionName, asIScriptFunction* cb);
		void bindOnInputReleased(uint32_t entity, const std::string& actionName, asIScriptFunction *cb);

		void releaseOnInputPressed(uint32_t entity, const std::string& actionName);
		void releaseOnInputReleased(uint32_t entity, const std::string& actionName);

		// Collision Functions
		void bindOnCollisionBegin(uint32_t entity, asIScriptFunction* cb);
		void bindOnCollisionEnd(uint32_t entity, asIScriptFunction* cb);

		void releaseOnCollisionBegin(uint32_t entity);
		void releaseOnCollisionEnd(uint32_t entity);
	};
}
