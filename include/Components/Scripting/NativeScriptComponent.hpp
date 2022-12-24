#pragma once

#include "ECS/Entity.h"
#include "nlohmann/json.hpp"

#include <memory>

using json = nlohmann::json;

namespace Puffin::Scripting
{
	// Base class for custom native script's
	class NativeScriptObject
	{
	public:

		NativeScriptObject(std::shared_ptr<ECS::Entity> entity) : m_entity(entity) {}
		virtual ~NativeScriptObject() { m_entity = nullptr; }

		virtual void Start() = 0;
		virtual void Update() = 0;
		virtual void Stop() = 0;

		std::shared_ptr<ECS::Entity> Entity() { return m_entity; }

	protected:

	private:

		std::shared_ptr<ECS::Entity> m_entity = nullptr; // Entity this script is attached to (private so it cannot be made nullptr by the inheriting script)

	};

	struct NativeScriptComponent
	{
		~NativeScriptComponent() { object = nullptr; }

		std::shared_ptr<NativeScriptObject> object = nullptr;

		int testVal = 0;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(NativeScriptComponent, testVal)
	};
}
