#pragma once

#define ANGELSCRIPT_DLL_LIBRARY_IMPORT
#include <angelscript.h>

#include <Components/BaseComponent.h>

#include <string>
#include <set>

#include <cereal/cereal.hpp>

namespace Puffin
{
	namespace Scripting
	{
		struct AngelScriptComponent : public BaseComponent
		{
			AngelScriptComponent()
			{
				bFlagCreated = true;
				bFlagDeleted = false;
			}

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

			// Set of class property indexes
			std::set<int> editableProperties;
			std::set<int> visibleProperties;
		};

		template<class Archive>
		void serialize(Archive& archive, AngelScriptComponent& comp)
		{
			archive(CEREAL_NVP(comp.name), CEREAL_NVP(comp.dir));
		}
	}
}