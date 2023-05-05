#pragma once

#define ANGELSCRIPT_DLL_LIBRARY_IMPORT
#include <angelscript/angelscript.h>
#include "nlohmann/json.hpp"

#include <string>
#include <set>
#include <unordered_map>
#include <filesystem>

namespace fs = std::filesystem;

namespace puffin::scripting
{
	struct SerializedScriptData
	{
		std::unordered_map<std::string, bool> boolProperties;
		std::unordered_map<std::string, int8_t> int8Properties;
		std::unordered_map<std::string, int16_t> int16Properties;
		std::unordered_map<std::string, int32_t> int32Properties;
		std::unordered_map<std::string, int64_t> int64Properties;
		std::unordered_map<std::string, uint8_t> uint8Properties;
		std::unordered_map<std::string, uint16_t> uint16Properties;
		std::unordered_map<std::string, uint32_t> uint32Properties;
		std::unordered_map<std::string, uint64_t> uint64Properties;
		std::unordered_map<std::string, float> floatProperties;
		std::unordered_map<std::string, double> doubleProperties;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(SerializedScriptData,
		boolProperties,
		int8Properties,
		int16Properties,
		int32Properties,
		int64Properties,
		uint8Properties,
		uint16Properties,
		uint32Properties,
		uint64Properties,
		floatProperties,
		doubleProperties)
	};

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

		// Script Functions
		asIScriptFunction* startFunc;
		asIScriptFunction* updateFunc;
		asIScriptFunction* stopFunc;

		// Set of class property indexes
		std::set<int> editableProperties;
		std::set<int> visibleProperties;

		SerializedScriptData serializedData;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(AngelScriptComponent, name, dir, serializedData)
	};

	// Export Script Data

	static void ExportPropertyToScriptData(const AngelScriptComponent& script, SerializedScriptData& scriptData, const int index)
	{
		const char* propertyName = script.obj->GetPropertyName(index);
		std::string propertyString = propertyName;
		int propertyType = script.obj->GetPropertyTypeId(index);

		// Add property to correct vector based on it's type
		if (propertyType == asTYPEID_BOOL)
		{
			bool boolProperty = *(bool*)script.obj->GetAddressOfProperty(index);

			scriptData.boolProperties[propertyString] = boolProperty;
		}
		else if (propertyType == asTYPEID_INT8)
		{
			int8_t intProperty = *(int8_t*)script.obj->GetAddressOfProperty(index);

			scriptData.int8Properties[propertyString] = intProperty;
		}
		else if (propertyType == asTYPEID_INT16)
		{
			int16_t intProperty = *(int16_t*)script.obj->GetAddressOfProperty(index);

			scriptData.int16Properties[propertyString] = intProperty;
		}
		else if (propertyType == asTYPEID_INT32)
		{
			int32_t intProperty = *(int32_t*)script.obj->GetAddressOfProperty(index);

			scriptData.int32Properties[propertyString] = intProperty;
		}
		else if (propertyType == asTYPEID_INT64)
		{
			int64_t intProperty = *(int64_t*)script.obj->GetAddressOfProperty(index);

			scriptData.int64Properties[propertyString] = intProperty;
		}
		else if (propertyType == asTYPEID_UINT8)
		{
			uint8_t intProperty = *(uint8_t*)script.obj->GetAddressOfProperty(index);

			scriptData.uint8Properties[propertyString] = intProperty;
		}
		else if (propertyType == asTYPEID_UINT16)
		{
			uint16_t intProperty = *(uint16_t*)script.obj->GetAddressOfProperty(index);

			scriptData.uint16Properties[propertyString] = intProperty;
		}
		else if (propertyType == asTYPEID_UINT32)
		{
			uint32_t intProperty = *(uint32_t*)script.obj->GetAddressOfProperty(index);

			scriptData.uint32Properties[propertyString] = intProperty;
		}
		else if (propertyType == asTYPEID_UINT64)
		{
			uint64_t intProperty = *(uint64_t*)script.obj->GetAddressOfProperty(index);

			scriptData.uint64Properties[propertyString] = intProperty;
		}
		else if (propertyType == asTYPEID_FLOAT)
		{
			float floatProperty = *(float*)script.obj->GetAddressOfProperty(index);

			scriptData.floatProperties[propertyString] = floatProperty;
		}
		else if (propertyType == asTYPEID_DOUBLE)
		{
			double doubleProperty = *(double*)script.obj->GetAddressOfProperty(index);

			scriptData.doubleProperties[propertyString] = doubleProperty;
		}
	}

	static void ExportPropertiesToScriptData(const AngelScriptComponent& script, SerializedScriptData& scriptData)
	{
		if (script.obj != 0 && script.type != 0)
		{
			// Clear all old properties
			scriptData.boolProperties.clear();
			scriptData.int8Properties.clear();
			scriptData.int16Properties.clear();
			scriptData.int32Properties.clear();
			scriptData.int64Properties.clear();
			scriptData.uint8Properties.clear();
			scriptData.uint16Properties.clear();
			scriptData.uint32Properties.clear();
			scriptData.uint64Properties.clear();
			scriptData.floatProperties.clear();
			scriptData.doubleProperties.clear();

			int propertyCount = script.type->GetPropertyCount();
			for (int index = 0; index < propertyCount; index++)
			{
				ExportPropertyToScriptData(script, scriptData, index);
			}
		}
	}

	static void ExportEditablePropertiesToScriptData(const AngelScriptComponent& script, SerializedScriptData& scriptData)
	{
		if (script.obj != 0 && script.type != 0)
		{
			// Clear all old properties
			scriptData.boolProperties.clear();
			scriptData.int8Properties.clear();
			scriptData.int16Properties.clear();
			scriptData.int32Properties.clear();
			scriptData.int64Properties.clear();
			scriptData.uint8Properties.clear();
			scriptData.uint16Properties.clear();
			scriptData.uint32Properties.clear();
			scriptData.uint64Properties.clear();
			scriptData.floatProperties.clear();
			scriptData.doubleProperties.clear();

			for (int index : script.editableProperties)
			{
				ExportPropertyToScriptData(script, scriptData, index);
			}
		}
	}

	// Import Script Data

	static void ImportScriptProperty(const AngelScriptComponent& script, SerializedScriptData& scriptData, const int index)
	{
		const char* propertyName = script.obj->GetPropertyName(index);
		std::string propertyString = propertyName;
		int propertyType = script.obj->GetPropertyTypeId(index);

		if (propertyType == asTYPEID_BOOL)
		{
			bool* boolProperty = (bool*)script.obj->GetAddressOfProperty(index);

			*boolProperty = scriptData.boolProperties[propertyString];
		}
		else if (propertyType == asTYPEID_INT8)
		{
			int8_t* intProperty = (int8_t*)script.obj->GetAddressOfProperty(index);

			*intProperty = scriptData.int8Properties[propertyString];
		}
		else if (propertyType == asTYPEID_INT16)
		{
			int16_t* intProperty = (int16_t*)script.obj->GetAddressOfProperty(index);

			*intProperty = scriptData.int16Properties[propertyString];
		}
		else if (propertyType == asTYPEID_INT32)
		{
			int32_t* intProperty = (int32_t*)script.obj->GetAddressOfProperty(index);

			*intProperty = scriptData.int32Properties[propertyString];
		}
		else if (propertyType == asTYPEID_INT64)
		{
			int64_t* intProperty = (int64_t*)script.obj->GetAddressOfProperty(index);

			*intProperty = scriptData.int64Properties[propertyString];
		}
		else if (propertyType == asTYPEID_UINT8)
		{
			uint8_t* intProperty = (uint8_t*)script.obj->GetAddressOfProperty(index);

			*intProperty = scriptData.uint8Properties[propertyString];
		}
		else if (propertyType == asTYPEID_UINT16)
		{
			uint16_t* intProperty = (uint16_t*)script.obj->GetAddressOfProperty(index);

			*intProperty = scriptData.uint16Properties[propertyString];
		}
		else if (propertyType == asTYPEID_UINT32)
		{
			uint32_t* intProperty = (uint32_t*)script.obj->GetAddressOfProperty(index);

			*intProperty = scriptData.uint32Properties[propertyString];
		}
		else if (propertyType == asTYPEID_UINT64)
		{
			uint64_t* intProperty = (uint64_t*)script.obj->GetAddressOfProperty(index);

			*intProperty = scriptData.uint64Properties[propertyString];
		}
		else if (propertyType == asTYPEID_FLOAT)
		{
			float* floatProperty = (float*)script.obj->GetAddressOfProperty(index);

			*floatProperty = scriptData.floatProperties[propertyString];
		}
		else if (propertyType == asTYPEID_DOUBLE)
		{
			double* doubleProperty = (double*)script.obj->GetAddressOfProperty(index);

			*doubleProperty = scriptData.doubleProperties[propertyString];
		}
	}

	static void ImportScriptProperties(const AngelScriptComponent& script, SerializedScriptData& scriptData)
	{
		if (script.obj != 0 && script.type != 0)
		{
			int propertyCount = script.type->GetPropertyCount();
			for (int index = 0; index < propertyCount; index++)
			{
				ImportScriptProperty(script, scriptData, index);
			}
		}
	}

	static void ImportEditableProperties(const AngelScriptComponent& script, SerializedScriptData& scriptData)
	{
		if (script.obj != 0 && script.type != 0)
		{
			for (int index : script.editableProperties)
			{
				ImportScriptProperty(script, scriptData, index);
			}
		}
	}
}