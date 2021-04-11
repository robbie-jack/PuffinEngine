#pragma once

#define ANGELSCRIPT_DLL_LIBRARY_IMPORT
#include <angelscript.h>

#include <string>

#include <cereal/cereal.hpp>

namespace Puffin
{
	namespace Scripting
	{
		struct AngelScriptComponent
		{
			// Name of script
			std::string name;

			// Location of File relative to project
			std::string dir;

			// Interface that describes instantiated type
			asITypeInfo* type;

			// Interface for instance of script object
			asIScriptObject* obj;

			// Interface to store this classes update function, as preparing it each frame is costly
			asIScriptFunction* updateFunc;
		};

		template<class Archive>
		void serialize(Archive& archive, AngelScriptComponent& comp)
		{
			archive(CEREAL_NVP(comp.name), CEREAL_NVP(comp.dir));
		}
	}
}