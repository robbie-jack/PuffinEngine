#pragma once

#include "ECS/ECS.h"

// AngelScript Includes
#define ANGELSCRIPT_DLL_LIBRARY_IMPORT
#include "angelscript/angelscript.h"
#include "angelscript/scriptbuilder/scriptbuilder.h"
#include "angelscript/scriptstdstring/scriptstdstring.h"

#include "Components/Scripting/AngelScriptComponent.hpp"

#include "Input/InputEvent.h"
#include "Physics/CollisionEvent.h"

#include "Types/RingBuffer.h"

#include "Audio/AudioSubsystem.h"

#include <unordered_map>
#include <map>
#include <memory>

namespace Puffin
{
	namespace Scripting
	{
		struct ScriptCallback
		{
			ECS::EntityID entity;
			asIScriptFunction* func = 0;
			void* object = 0;
			asITypeInfo* objectType = 0;
		};

		typedef std::map<ECS::EntityID, ScriptCallback> ScriptCallbackMap;

		class AngelScriptSystem : public ECS::System
		{
		public:

			AngelScriptSystem();
			~AngelScriptSystem() override;

			void Init() override;
			void PreStart() override;
			void Start() override;
			void Update() override;
			void Stop() override;
			void Cleanup() override {}

			// Hot-Reloads all scripts when called
			void Reload() {}

		private:

			asIScriptEngine* m_scriptEngine = nullptr;
			asIScriptContext* m_ctx = nullptr;

			std::shared_ptr<Audio::AudioSubsystem> m_audioSubsystem;

			ECS::EntityID m_currentEntityID = 0; // Entity ID for currently executing script

			// Event Buffers
			std::shared_ptr<RingBuffer<Input::InputEvent>> m_inputEvents = nullptr;;
			std::shared_ptr<RingBuffer<Physics::CollisionBeginEvent>> m_collisionBeginEvents = nullptr;
			std::shared_ptr<RingBuffer<Physics::CollisionEndEvent>> m_collisionEndEvents = nullptr;

			// Maps of Input Callbacks
			std::unordered_map<std::string, ScriptCallbackMap> m_onInputPressedCallbacks;
			std::unordered_map<std::string, ScriptCallbackMap> m_onInputReleasedCallbacks;

			// Collision Callbacks
			ScriptCallbackMap m_onCollisionBeginCallbacks;
			ScriptCallbackMap m_onCollisionEndCallbacks;

			bool m_firstInitialize = true;

			void ConfigureEngine();

			void InitializeScript(ECS::EntityID entity, AngelScriptComponent& script);
			void CompileScript(AngelScriptComponent& script);
			void UpdateScriptMethods(AngelScriptComponent& script);
			void InstantiateScriptObj(ECS::EntityID entity, AngelScriptComponent& script);

			void CleanupScriptComponent(AngelScriptComponent& script);

			void ProcessEvents();

			asIScriptFunction* GetScriptMethod(const AngelScriptComponent& script, const char* funcName);
			bool PrepareScriptMethod(void* scriptObj, asIScriptFunction* scriptFunc);
			bool ExecuteScriptMethod(void* scriptObj, asIScriptFunction* scriptFunc);
			bool PrepareAndExecuteScriptMethod(void* scriptObj, asIScriptFunction* scriptFunc);

			// Global Script Functions
			const double& GetDeltaTime() const;
			const double& GetFixedTime() const;

			void PlaySoundEffect(uint64_t id, float volume = 1.0f, bool looping = false, bool restart = false);
			uint64_t PlaySoundEffect(const std::string& path, float volume = 1.0f, bool looping = false, bool restart = false);

			int GetEntityID(); // Return the Entity ID for the attached script

			// Script Callbacks
			ScriptCallback BindCallback(uint32_t entity, asIScriptFunction* cb) const;
			void ReleaseCallback(ScriptCallback& scriptCallback) const;

			// Input Functions
			void BindOnInputPressed(uint32_t entity, const std::string& actionName, asIScriptFunction* cb);
			void BindOnInputReleased(uint32_t entity, const std::string& actionName, asIScriptFunction *cb);

			void ReleaseOnInputPressed(uint32_t entity, const std::string& actionName);
			void ReleaseOnInputReleased(uint32_t entity, const std::string& actionName);

			// Collision Functions
			void BindOnCollisionBegin(uint32_t entity, asIScriptFunction* cb);
			void BindOnCollisionEnd(uint32_t entity, asIScriptFunction* cb);

			void ReleaseOnCollisionBegin(uint32_t entity);
			void ReleaseOnCollisionEnd(uint32_t entity);
		};
	}
}