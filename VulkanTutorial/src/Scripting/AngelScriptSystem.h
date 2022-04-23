#pragma once

#include <ECS/ECS.h>

// AngelScript Includes
#define ANGELSCRIPT_DLL_LIBRARY_IMPORT
#include <angelscript.h>
#include <scriptbuilder/scriptbuilder.h>
#include <scriptstdstring/scriptstdstring.h>

#include <Components/AngelScriptComponent.h>

#include "Input/InputEvent.h"

#include <Types/RingBuffer.h>

#include <unordered_map>
#include <map>

namespace Puffin
{
	namespace Scripting
	{
		struct ScriptCallback
		{
			ECS::Entity entity;
			asIScriptFunction* func = 0;
			void* object = 0;
			asITypeInfo* objectType = 0;
		};

		typedef std::map<ECS::Entity, ScriptCallback> ScriptCallbackMap;

		class AngelScriptSystem : public ECS::System
		{
		public:
			void Init() override;

			void PreStart() override;

			void Start() override;

			void Update() override;

			void Stop() override;

			void Cleanup() override;

			// Hot-Reloads all scripts when called
			void Reload();

			ECS::SystemInfo GetInfo() override
			{
				ECS::SystemInfo info;

				info.updateOrder = ECS::UpdateOrder::Update;

				return info;
			}

		private:

			asIScriptEngine* m_scriptEngine;
			asIScriptContext* m_ctx;

			ECS::Entity m_currentEntityID; // Entity ID for currently executing script

			// Event Buffers
			std::shared_ptr<RingBuffer<Input::InputEvent>> m_inputEvents;

			// Maps of Input Callbacks
			std::unordered_map<std::string, ScriptCallbackMap> m_onInputPressedCallbacks;
			std::unordered_map<std::string, ScriptCallbackMap> m_onInputReleasedCallbacks;

			void ConfigureEngine();

			void InitializeScript(ECS::Entity entity, AngelScriptComponent& script);
			void CompileScript(AngelScriptComponent& script);
			void UpdateScriptMethods(AngelScriptComponent& script);
			void InstantiateScriptObj(ECS::Entity entity, AngelScriptComponent& script);

			void CleanupScriptComponent(AngelScriptComponent& script);

			void ProcessEvents();

			asIScriptFunction* GetScriptMethod(const AngelScriptComponent& script, const char* funcName);
			bool PrepareScriptMethod(void* scriptObj, asIScriptFunction* scriptFunc);
			bool ExecuteScriptMethod(void* scriptObj, asIScriptFunction* scriptFunc);
			bool PrepareAndExecuteScriptMethod(void* scriptObj, asIScriptFunction* scriptFunc);

			// Global Script Functions
			double GetDeltaTime();
			double GetFixedTime();

			int GetEntityID(); // Return the Entity ID for the attached script

			// Script Callbacks
			void ExecuteCallback(const ScriptCallback& callback);
			ScriptCallback BindCallback(uint32_t entity, asIScriptFunction* cb);
			void ReleaseCallback(ScriptCallback& scriptCallback);

			// Input Functions
			void BindOnInputPressed(uint32_t entity, const std::string& actionName, asIScriptFunction* cb);
			void BindOnInputReleased(uint32_t entity, const std::string& actionName, asIScriptFunction *cb);

			void ReleaseOnInputPressed(uint32_t entity, const std::string& actionName);
			void ReleaseOnInputReleased(uint32_t entity, const std::string& actionName);
		};
	}
}