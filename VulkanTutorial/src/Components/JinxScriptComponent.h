#pragma once

#include <Jinx.hpp>
#include <string>

#include <Components/BaseComponent.h>

#include <cereal/cereal.hpp>

namespace Puffin
{
	namespace Scripting
	{
		struct JinxScriptComponent : public BaseComponent
		{
			// Name of the file where script is stored as plain text
			std::string Name;

			// Location of File relative to project
			std::string Dir;

			// Compiled bytecode of the script
			Jinx::BufferPtr Bytecode;

			// Jinx Script object ready to be executed
			Jinx::ScriptPtr Script;
		};

		template<class Archive>
		void serialize(Archive& archive, JinxScriptComponent& comp)
		{
			archive(CEREAL_NVP(comp.Name), CEREAL_NVP(comp.Dir));
		}
	}
}