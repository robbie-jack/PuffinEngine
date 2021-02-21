#pragma once

#include <ECS/ECS.h>
#include <Jinx.hpp>
#include <Components/JinxScriptComponent.h>
#include <unordered_map>

namespace Puffin
{
	namespace Scripting
	{
		class JinxScriptingSystem : public ECS::System
		{
		public:
			void Init();
	
			bool Update(float dt);
	
			void Cleanup();
	
		private:
	
			//Jinx::BufferPtr bytecode;
			Jinx::RuntimePtr runtime;
			Jinx::BufferPtr puffinCoreBytecode;

			std::unordered_map<std::string_view, JinxScriptComponent> libraryComponents;

			void InitLibraries();

			void InitComponent(JinxScriptComponent& comp);
		};
	}
}