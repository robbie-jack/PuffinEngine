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

		std::shared_ptr<core::Engine> m_engine;
		asIScriptEngine* m_script_engine;

		double mDeltaTime;
		double mFixedTime;

		// Maps of Input Callbacks
		std::unordered_map<std::string, ScriptCallbackMap> mOnInputPressedCallbacks;
		std::unordered_map<std::string, ScriptCallbackMap> mOnInputReleasedCallbacks;

		// Collision Callbacks
		ScriptCallbackMap mOnCollisionBeginCallbacks;
		ScriptCallbackMap mOnCollisionEndCallbacks;

		// Register global methods with the script engine
		void registerGlobalMethods();

		// Global Script Functions
		[[nodiscard]] const double& getDeltaTime();
		[[nodiscard]] const double& getFixedTime();

		TransformComponent3D& getTransformComponent3D(UUID id) const;
		bool hasTransformComponent3D(UUID id) const;

		template<typename T>
		T& getComponent(UUID id) const;

		template<typename T>
		bool hasComponent(UUID id) const;

		// Script Callbacks
		ScriptCallback bindCallback(UUID entity, asIScriptFunction* cb) const;
		void releaseCallback(ScriptCallback& scriptCallback) const;

		// Input Functions
		void bindOnInputPressed(UUID entity, const std::string& actionName, asIScriptFunction* cb);
		void bindOnInputReleased(UUID entity, const std::string& actionName, asIScriptFunction* cb);

		void releaseOnInputPressed(UUID entity, const std::string& actionName);
		void releaseOnInputReleased(UUID entity, const std::string& actionName);
	};
}
