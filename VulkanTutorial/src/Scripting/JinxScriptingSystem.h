#pragma once

#include <ECS/ECS.h>
#include <Jinx.hpp>

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
	
			Jinx::BufferPtr bytecode;
			Jinx::RuntimePtr runtime;
		};
	}
}