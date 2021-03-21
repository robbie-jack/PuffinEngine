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

			void Start();
	
			bool Update(float dt);

			void Stop();
	
		private:
	
			//Jinx::BufferPtr bytecode;
			Jinx::RuntimePtr runtime;
			Jinx::BufferPtr puffinCoreBytecode;

			std::unordered_map<std::string_view, JinxScriptComponent> libraryComponents;

			std::shared_ptr<RingBuffer<JinxScriptEvent>> scriptEvents;

			void InitLibraries();

			void InitComponent(JinxScriptComponent& comp);

			void ProcessEvents();
		};
	}
}