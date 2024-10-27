#pragma once

#include <memory>

#include "puffin/components/scripting/angelscriptcomponent.h"
#include "puffin/types/uuid.h"

class asIScriptEngine;

namespace puffin
{
	struct TransformComponent3D;

	namespace core
	{
		class Engine;
	}
}

namespace puffin::scripting
{
	class AngelScriptSubsystem;

	struct ScriptCallback
	{
		UUID entity;
		asIScriptFunction* func = nullptr;
		void* object = nullptr;
		asITypeInfo* objectType = nullptr;
	};

	typedef std::map<UUID, ScriptCallback> ScriptCallbackMap;

	/*
	 *	Class which defines various methods used by AngelScript scripts as global methods to access engine functionality
	 *	including ecs, input, audio, etc...
	 */
	class AngelScriptEngineInterface
	{
	public:

		AngelScriptEngineInterface(std::shared_ptr<core::Engine> engine, asIScriptEngine* script_engine);

		~AngelScriptEngineInterface();

	private:

		// Register global methods with the script engine
		void RegisterGlobalMethods();

		// Global Script Functions
		[[nodiscard]] const double& GetDeltaTime();
		[[nodiscard]] const double& GetFixedTime();

		TransformComponent3D& GetTransformComponent3D(UUID id) const;
		bool HasTransformComponent3D(UUID id) const;

		template<typename T>
		T& getComponent(UUID id) const;

		template<typename T>
		bool HasComponent(UUID id) const;

		// Script Callbacks
		ScriptCallback BindCallback(UUID entity, asIScriptFunction* cb) const;
		void ReleaseCallback(ScriptCallback& scriptCallback) const;

		// Input Functions
		void BindOnInputPressed(UUID entity, const std::string& actionName, asIScriptFunction* cb);
		void BindOnInputReleased(UUID entity, const std::string& actionName, asIScriptFunction* cb);

		void ReleaseOnInputPressed(UUID entity, const std::string& actionName);
		void ReleaseOnInputReleased(UUID entity, const std::string& actionName);

		std::shared_ptr<core::Engine> mEngine;
		asIScriptEngine* mScriptEngine;

		double mDeltaTime;
		double mFixedTime;

		// Maps of Input Callbacks
		std::unordered_map<std::string, ScriptCallbackMap> mOnInputPressedCallbacks;
		std::unordered_map<std::string, ScriptCallbackMap> mOnInputReleasedCallbacks;

		// Collision Callbacks
		ScriptCallbackMap mOnCollisionBeginCallbacks;
		ScriptCallbackMap mOnCollisionEndCallbacks;

	};
}
