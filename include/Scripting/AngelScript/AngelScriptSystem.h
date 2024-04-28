#pragma once

// AngelScript Includes
#include "angelscript/angelscript.h"

#include "Core/System.h"
#include "Components/Scripting/AngelScriptComponent.h"
#include "Core/Engine.h"
#include "Input/InputEvent.h"
#include "Physics/CollisionEvent.h"
#include "Types/RingBuffer.h"
#include "Types/UUID.h"
#include "Scripting/AngelScript/AngelScriptEngineInterface.h"

#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <entt/entity/registry.hpp>

#include "puffin/audio/audio_subsystem.h"

namespace puffin::scripting
{
	class AngelScriptSystem : public core::System, public std::enable_shared_from_this<AngelScriptSystem>
	{
	public:

		AngelScriptSystem(const std::shared_ptr<core::Engine>& engine);
		~AngelScriptSystem() override;

		void startup();
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

		std::unique_ptr<AngelScriptEngineInterface> mEngineInterface;
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

		// Collision Functions
		void bindOnCollisionBegin(PuffinID entity, asIScriptFunction* cb);
		void bindOnCollisionEnd(PuffinID entity, asIScriptFunction* cb);

		void releaseOnCollisionBegin(PuffinID entity);
		void releaseOnCollisionEnd(PuffinID entity);
	};
}
