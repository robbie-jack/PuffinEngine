#pragma once

#include <Jinx.hpp>
#include <string>

#include <Components/BaseComponent.h>

namespace Puffin
{
	namespace Scripting
	{
		struct JinxScriptComponent : public BaseComponent
		{
			// Name of the file where script is stored as plain text
			std::string Name;

			// Compiled bytecode of the script
			Jinx::BufferPtr Bytecode;

			// Jinx Script object ready to be executed
			Jinx::ScriptPtr Script;
		};
	}
}