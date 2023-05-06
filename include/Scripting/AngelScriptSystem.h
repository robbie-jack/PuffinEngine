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

namespace puffin
{
	namespace scripting
	{
		struct ScriptCallback
		{
			PuffinId entity;
			asIScriptFunction* func = nullptr;
			void* object = nullptr;
			asITypeInfo* objectType = nullptr;
		};

		typedef std::map<PuffinId, ScriptCallback> ScriptCallbackMap;

		class AngelScriptSystem : public core::System
		{
		public:

			AngelScriptSystem();
			~AngelScriptSystem() override;

			void setupCallbacks() override
			{
				mEngine->registerCallback(core::ExecutionStage::Init, [&]() { Init(); }, "AngelScriptSystem: Init");
				mEngine->registerCallback(core::ExecutionStage::Setup, [&]() { Setup(); }, "AngelScriptSystem: Setup");
				mEngine->registerCallback(core::ExecutionStage::Start, [&]() { Start(); }, "AngelScriptSystem: Start");
				mEngine->registerCallback(core::ExecutionStage::Update, [&]() { Update(); }, "AngelScriptSystem: Update");
				mEngine->registerCallback(core::ExecutionStage::Stop, [&]() { Stop(); }, "AngelScriptSystem: Stop");
			}

			void Init();
			void Setup();
			void Start();
			void Update();
			void Stop();

			// Hot-Reloads all scripts when called
			void Reload() {}

		private:

			asIScriptEngine* m_scriptEngine = nullptr;
			asIScriptContext* m_ctx = nullptr;

			std::shared_ptr<audio::AudioSubsystem> m_audioSubsystem;

			PuffinId m_currentEntityID; // Entity ID for currently executing script

			// Event Buffers
			std::shared_ptr<RingBuffer<input::InputEvent>> m_inputEvents = nullptr;;
			std::shared_ptr<RingBuffer<physics::CollisionBeginEvent>> m_collisionBeginEvents = nullptr;
			std::shared_ptr<RingBuffer<physics::CollisionEndEvent>> m_collisionEndEvents = nullptr;

			// Maps of Input Callbacks
			std::unordered_map<std::string, ScriptCallbackMap> m_onInputPressedCallbacks;
			std::unordered_map<std::string, ScriptCallbackMap> m_onInputReleasedCallbacks;

			// Collision Callbacks
			ScriptCallbackMap m_onCollisionBeginCallbacks;
			ScriptCallbackMap m_onCollisionEndCallbacks;

			bool m_firstInitialize = true;

			void ConfigureEngine();

			void InitializeScript(PuffinId entity, AngelScriptComponent& script);
			void CompileScript(AngelScriptComponent& script);
			void UpdateScriptMethods(AngelScriptComponent& script);
			void InstantiateScriptObj(PuffinId entity, AngelScriptComponent& script);

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

			uint64_t GetEntityID(); // Return the Entity ID for the attached script

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