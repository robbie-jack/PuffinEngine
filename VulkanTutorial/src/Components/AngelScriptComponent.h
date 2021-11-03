#pragma once

#define ANGELSCRIPT_DLL_LIBRARY_IMPORT
#include <angelscript.h>

#include <cereal/cereal.hpp>

#include <string>
#include <set>
#include <filesystem>

namespace fs = std::filesystem;

namespace Puffin
{
	namespace Scripting
	{
		struct AngelScriptComponent
		{
			// Name of script
			std::string name;

			// Location of File relative to project
			fs::path dir;

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
		void save(Archive& archive, const AngelScriptComponent& comp)
		{
			archive(CEREAL_NVP(comp.name), CEREAL_NVP(comp.dir.string()));
		}

		template<class Archive>
		void load(Archive& archive, AngelScriptComponent& comp)
		{
			std::string dir;
			archive(CEREAL_NVP(comp.name), CEREAL_NVP(dir));

			comp.dir = dir;
		}
	}
}