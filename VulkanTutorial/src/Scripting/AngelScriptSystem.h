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

			void Start() override;

			void Update() override;

			void Stop() override;

			void Cleanup() override;

			ECS::SystemInfo GetInfo() override
			{
				ECS::SystemInfo info;

				info.updateOrder = ECS::UpdateOrder::Update;

				return info;
			}

		private:

			asIScriptEngine* scriptEngine;
			asIScriptContext* ctx;

			void ConfigureEngine();

			void InitScriptComponent(AngelScriptComponent& script);
			void CleanupScriptComponent(AngelScriptComponent& script);

		};
	}
}