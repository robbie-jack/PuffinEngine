#pragma once

#include <memory>

#include "Components/Scripting/AngelScriptComponent.h"
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
	class AngelScriptSystem;

	struct ScriptCallback
	{
		PuffinID entity;
		asIScriptFunction* func = nullptr;
		void* object = nullptr;
		asITypeInfo* objectType = nullptr;
	};

	typedef std::map<PuffinID, ScriptCallback> ScriptCallbackMap;

	/*
	 *	Class which defines various methods used by AngelScript scripts as global methods to access engine functionality
	 *	including ecs, input, audio, etc...
	 */
	class AngelScriptEngineInterface : public std::enable_shared_from_this<AngelScriptEngineInterface>
	{
	public:

		AngelScriptEngineInterface(std::shared_ptr<core::Engine> engine, std::shared_ptr<AngelScriptSystem> scriptSystem, asIScriptEngine* scriptEngine);

		~AngelScriptEngineInterface();

	private:

		std::shared_ptr<core::Engine> mEngine;
		std::shared_ptr<scripting::AngelScriptSystem> mScriptSystem;
		asIScriptEngine* mScriptEngine;

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

		TransformComponent3D& getTransformComponent3D(PuffinID id) const;
		bool hasTransformComponent3D(PuffinID id) const;

		template<typename T>
		T& getComponent(PuffinID id) const;

		template<typename T>
		bool hasComponent(PuffinID id) const;

		// Script Callbacks
		ScriptCallback bindCallback(PuffinID entity, asIScriptFunction* cb) const;
		void releaseCallback(ScriptCallback& scriptCallback) const;

		// Input Functions
		void bindOnInputPressed(PuffinID entity, const std::string& actionName, asIScriptFunction* cb);
		void bindOnInputReleased(PuffinID entity, const std::string& actionName, asIScriptFunction* cb);

		void releaseOnInputPressed(PuffinID entity, const std::string& actionName);
		void releaseOnInputReleased(PuffinID entity, const std::string& actionName);
	};
}
