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
			void Init();

			void Start();

			bool Update(float dt);

			void Stop();

			void Cleanup();

		private:

			asIScriptEngine* scriptEngine;
			asIScriptContext* ctx;

			void ConfigureEngine();

			void InitScriptComponent(AngelScriptComponent& script);
			void CleanupScriptComponent(AngelScriptComponent& script);

		};
	}
}