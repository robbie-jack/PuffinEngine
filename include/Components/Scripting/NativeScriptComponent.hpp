#pragma once

#include "Types/UUID.h"
#include "nlohmann/json.hpp"

#include <memory>

using json = nlohmann::json;

namespace puffin::scripting
{
	// Base class for custom native script's
	class NativeScriptObject
	{
	public:

		NativeScriptObject(UUID id) : mId(id) {}
		virtual ~NativeScriptObject() {  }

		const UUID& id() const { return mId; }

	protected:

	private:

		UUID mId; // Entity this script is attached to (private so it cannot be made nullptr by the inheriting script)

	};

	struct NativeScriptComponent
	{
		~NativeScriptComponent() { object = nullptr; }

		std::shared_ptr<NativeScriptObject> object = nullptr;

		int testVal = 0;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(NativeScriptComponent, testVal)
	};
}
