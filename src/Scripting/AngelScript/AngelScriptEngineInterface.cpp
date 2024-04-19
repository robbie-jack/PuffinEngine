#include "Scripting/AngelScript/AngelScriptEngineInterface.h"

#include "Core/Engine.h"
#include "Components/TransformComponent3D.h"
#include "Scripting/AngelScript/AngelScriptSystem.h"
#include "angelscript/angelscript.h"
#include "ECS/EnTTSubsystem.h"

namespace puffin::scripting
{
	AngelScriptEngineInterface::AngelScriptEngineInterface(std::shared_ptr<core::Engine> engine, std::shared_ptr<AngelScriptSystem> scriptSystem,
		asIScriptEngine* scriptEngine) : mEngine(engine), mScriptSystem(scriptSystem), mScriptEngine(scriptEngine)
	{
		mScriptEngine->AddRef();

		registerGlobalMethods();
	}

	AngelScriptEngineInterface::~AngelScriptEngineInterface()
	{
		mEngine = nullptr;

		mScriptEngine->Release();
		mScriptEngine = nullptr;
	}

	void AngelScriptEngineInterface::registerGlobalMethods()
	{
		int r = 0;

		r = mScriptEngine->RegisterGlobalFunction("double GetDeltaTime()", asMETHOD(AngelScriptEngineInterface, getDeltaTime), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = mScriptEngine->RegisterGlobalFunction("double GetFixedTime()", asMETHOD(AngelScriptEngineInterface, getFixedTime), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);

		// Register component globals
		r = mScriptEngine->RegisterGlobalFunction("TransformComponent3D@ GetTransform3D(uint64)",
			asMETHOD(AngelScriptEngineInterface, getComponent<TransformComponent3D>), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);

		r = mScriptEngine->RegisterGlobalFunction("bool HasTransform3D(uint64)",
			asMETHOD(AngelScriptEngineInterface, hasComponent<TransformComponent3D>), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);

		// Register Input Funcdefs and Bind/Release Callback Methods
		r = mScriptEngine->RegisterFuncdef("void OnInputPressedCallback()"); assert(r >= 0);
		r = mScriptEngine->RegisterFuncdef("void OnInputReleasedCallback()"); assert(r >= 0);

		r = mScriptEngine->RegisterGlobalFunction("void BindOnInputPressed(uint64, const string &in, OnInputPressedCallback @cb)", asMETHOD(AngelScriptEngineInterface, bindOnInputPressed), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = mScriptEngine->RegisterGlobalFunction("void BindOnInputReleased(uint64, const string &in, OnInputReleasedCallback @cb)", asMETHOD(AngelScriptEngineInterface, bindOnInputReleased), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = mScriptEngine->RegisterGlobalFunction("void ReleaseOnInputPressed(uint64, const string &in)", asMETHOD(AngelScriptEngineInterface, releaseOnInputPressed), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = mScriptEngine->RegisterGlobalFunction("void ReleaseOnInputReleased(uint64, const string &in)", asMETHOD(AngelScriptEngineInterface, releaseOnInputReleased), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
	}

	const double& AngelScriptEngineInterface::getDeltaTime()
	{
		mDeltaTime = mEngine->deltaTime();
		return mDeltaTime;
	}

	const double& AngelScriptEngineInterface::getFixedTime()
	{
		mFixedTime = mEngine->timeStepFixed();
		return mFixedTime;
	}

	TransformComponent3D& AngelScriptEngineInterface::getTransformComponent3D(PuffinID id) const
	{
		const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

		entt::entity entity = mEngine->getSystem<ecs::EnTTSubsystem>()->get_entity(id);

		registry->patch<TransformComponent3D>(entity, [&](auto& transform){});

		return registry->get<TransformComponent3D>(entity);
	}

	bool AngelScriptEngineInterface::hasTransformComponent3D(PuffinID id) const
	{
		const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

		entt::entity entity = mEngine->getSystem<ecs::EnTTSubsystem>()->get_entity(id);

		if (registry->any_of<TransformComponent3D>(entity))
		{
			return true;
		}

		return false;
	}

	template <typename T>
	T& AngelScriptEngineInterface::getComponent(PuffinID id) const
	{
		const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

		entt::entity entity = mEngine->getSystem<ecs::EnTTSubsystem>()->get_entity(id);

		registry->patch<T>(entity, [&](auto& transform) {});

		return registry->get<T>(entity);
	}

	template <typename T>
	bool AngelScriptEngineInterface::hasComponent(PuffinID id) const
	{
		const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

		entt::entity entity = mEngine->getSystem<ecs::EnTTSubsystem>()->get_entity(id);

		if (registry->any_of<T>(entity))
		{
			return true;
		}

		return false;
	}

	ScriptCallback AngelScriptEngineInterface::bindCallback(PuffinID entity, asIScriptFunction* cb) const
	{
		ScriptCallback scriptCallback;
		scriptCallback.entity = entity;

		if (cb && cb->GetFuncType() == asFUNC_DELEGATE)
		{
			// Store object, type and callback
			scriptCallback.object = cb->GetDelegateObject();
			scriptCallback.objectType = cb->GetDelegateObjectType();
			scriptCallback.func = cb->GetDelegateFunction();

			// Increment Refs
			mScriptEngine->AddRefScriptObject(scriptCallback.object, scriptCallback.objectType);
			scriptCallback.func->AddRef();

			// Release Delegate
			cb->Release();
		}
		else
		{
			//Store handle for later use
			scriptCallback.func = cb;
		}

		return scriptCallback;
	}

	void AngelScriptEngineInterface::releaseCallback(ScriptCallback& scriptCallback) const
	{
		if (scriptCallback.func)
			scriptCallback.func->Release();

		if (scriptCallback.object)
			mScriptEngine->ReleaseScriptObject(scriptCallback.object, scriptCallback.objectType);

		scriptCallback.func = nullptr;
		scriptCallback.object = nullptr;
		scriptCallback.objectType = nullptr;
	}

	void AngelScriptEngineInterface::bindOnInputPressed(PuffinID entity, const std::string& actionName,
		asIScriptFunction* cb)
	{
		// Release existing callback function, if one exists
		releaseOnInputPressed(entity, actionName);

		mOnInputPressedCallbacks[actionName][entity] = bindCallback(entity, cb);
	}

	void AngelScriptEngineInterface::bindOnInputReleased(PuffinID entity, const std::string& actionName,
		asIScriptFunction* cb)
	{
		// Release existing callback function, if one exists
		releaseOnInputReleased(entity, actionName);

		mOnInputPressedCallbacks[actionName][entity] = bindCallback(entity, cb);
	}

	void AngelScriptEngineInterface::releaseOnInputPressed(PuffinID entity, const std::string& actionName)
	{
		if (mOnInputPressedCallbacks[actionName].count(entity))
		{
			releaseCallback(mOnInputPressedCallbacks[actionName][entity]);
			mOnInputPressedCallbacks[actionName].erase(entity);
		}
	}

	void AngelScriptEngineInterface::releaseOnInputReleased(PuffinID entity, const std::string& actionName)
	{
		if (mOnInputPressedCallbacks[actionName].count(entity))
		{
			releaseCallback(mOnInputPressedCallbacks[actionName][entity]);
			mOnInputPressedCallbacks[actionName].erase(entity);
		}
	}
}
