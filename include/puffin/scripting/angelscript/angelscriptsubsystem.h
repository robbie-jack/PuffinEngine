#pragma once

#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <entt/entity/registry.hpp>

// AngelScript Includes
#include "angelscript/angelscript.h"

#include "puffin/core/subsystem.h"
#include "puffin/core/engine.h"
#include "puffin/input/inputevent.h"
#include "puffin/types/storage/ringbuffer.h"
#include "puffin/types/uuid.h"

#include "puffin/scripting/angelscript/angelscriptengineinterface.h"
#include "puffin/audio/audiosubsystem.h"
#include "puffin/physics/collisionevent.h"

namespace puffin::scripting
{
	class AngelScriptSubsystem : public core::Subsystem, public std::enable_shared_from_this<AngelScriptSubsystem>
	{
	public:

		explicit AngelScriptSubsystem(const std::shared_ptr<core::Engine>& engine);
		~AngelScriptSubsystem() override;

		void Initialize(core::SubsystemManager* subsystemManager) override;
		void Deinitialize() override;

		void EndPlay() override;

		void Update(double deltaTime) override;
		bool ShouldUpdate() override;

		void OnConstructScript(entt::registry& registry, entt::entity entity);
		void OnDestroyScript(entt::registry& registry, entt::entity entity);

		bool PrepareAndExecuteScriptMethod(void* script_obj, asIScriptFunction* script_func);

		void SetCurrentEntityID(UUID id);

		// Hot-Reloads all scripts when called
		//void reload() {}

	private:

		void ConfigureEngine();

		void InitContext();
		void InitScripts();
		void StartScripts();
		void StopScripts();

		void InitializeScript(UUID entity, AngelScriptComponent& script);
		void CompileScript(AngelScriptComponent& script) const;
		void UpdateScriptMethods(AngelScriptComponent& script);
		void InstantiateScriptObj(UUID entity, AngelScriptComponent& script);

		void DestroyScript(AngelScriptComponent& script);

		void ProcessEvents();

		asIScriptFunction* GetScriptMethod(const AngelScriptComponent& script, const char* funcName);
		bool PrepareScriptMethod(void* scriptObj, asIScriptFunction* scriptFunc);
		bool ExecuteScriptMethod(void* scriptObj, asIScriptFunction* scriptFunc);

		// Global Script Functions
		[[nodiscard]] const double& GetDeltaTime() const;
		[[nodiscard]] const double& GetFixedTime() const;

		void PlaySoundEffect(uint64_t id, float volume = 1.0f, bool looping = false, bool restart = false);
		UUID PlaySoundEffect(const std::string& path, float volume = 1.0f, bool looping = false,
			bool restart = false);

		UUID GetEntityID(); // Return the Entity ID for the attached script

		// Script Callbacks
		ScriptCallback BindCallback(UUID entity, asIScriptFunction* cb) const;
		void ReleaseCallback(ScriptCallback& scriptCallback) const;

		// Collision Functions
		void BindOnCollisionBegin(UUID entity, asIScriptFunction* cb);
		void BindOnCollisionEnd(UUID entity, asIScriptFunction* cb);

		void ReleaseOnCollisionBegin(UUID entity);
		void ReleaseOnCollisionEnd(UUID entity);

		asIScriptEngine* mScriptEngine = nullptr;
		asIScriptContext* mScriptContext = nullptr;

		std::unique_ptr<AngelScriptEngineInterface> mEngineInterface;

		UUID mCurrentEntityID; // Entity ID for currently executing script

		// Event Buffers
		std::shared_ptr<RingBuffer<input::InputEvent>> mInputEvents = nullptr;
		std::shared_ptr<RingBuffer<physics::CollisionBeginEvent>> mCollisionBeginEvents = nullptr;
		std::shared_ptr<RingBuffer<physics::CollisionEndEvent>> mCollisionEndEvents = nullptr;

		// Maps of Input Callbacks
		std::unordered_map<std::string, ScriptCallbackMap> mOnInputPressedCallbacks;
		std::unordered_map<std::string, ScriptCallbackMap> mOnInputReleasedCallbacks;

		// Collision Callbacks
		ScriptCallbackMap mOnCollisionBeginCallbacks;
		ScriptCallbackMap mOnCollisionEndCallbacks;

		std::unordered_set<UUID> mScriptsToInit;
		std::unordered_set<UUID> mScriptsToBeginPlay;
		std::unordered_set<UUID> mScriptsToEndPlay;
	};

	class AngelScriptGameplaySubsystem : public core::Subsystem
	{
	public:

		explicit AngelScriptGameplaySubsystem(std::shared_ptr<core::Engine> engine);
		~AngelScriptGameplaySubsystem() override = default;

		[[nodiscard]] core::SubsystemType GetType() const override;

		void BeginPlay() override;
		void EndPlay() override;

		void Update(double deltaTime) override;
		bool ShouldUpdate() override;

		void FixedUpdate(double fixedTime) override;
		bool ShouldFixedUpdate() override;

	private:



	};
}
