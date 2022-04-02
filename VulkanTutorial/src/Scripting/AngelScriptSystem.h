#pragma once

#include <ECS/ECS.h>

// AngelScript Includes
#define ANGELSCRIPT_DLL_LIBRARY_IMPORT
#include <angelscript.h>
#include <scriptbuilder/scriptbuilder.h>
#include <scriptstdstring/scriptstdstring.h>

#include <Components/AngelScriptComponent.h>

namespace Puffin
{
	namespace Scripting
	{
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

			void ConfigureEngine();

			void InitializeScript(AngelScriptComponent& script);
			void CompileScript(AngelScriptComponent& script);
			void InstantiateScriptObj(AngelScriptComponent& script);

			void CleanupScriptComponent(AngelScriptComponent& script);

			asIScriptFunction* GetScriptMethod(const AngelScriptComponent& script, const char* funcName);
			bool ExecuteScriptMethod(asIScriptObject* scriptObj, asIScriptFunction* scriptFunc);

			// Global Script Functions
			double GetDeltaTime();
			double GetFixedTime();
		};
	}
}