#include "puffin/scripting/angelscript/angelscriptengineinterface.h"

#include "puffin/core/engine.h"
#include "puffin/components/transformcomponent3d.h"
#include "puffin/scripting/angelscript/angelscriptsubsystem.h"
#include "angelscript/angelscript.h"
#include "puffin/ecs/enttsubsystem.h"

namespace puffin::scripting
{
	AngelScriptEngineInterface::AngelScriptEngineInterface(std::shared_ptr<core::Engine> engine, asIScriptEngine* scriptEngine) :
		mEngine(engine), mScriptEngine(scriptEngine)
	{
		mScriptEngine->AddRef();

		RegisterGlobalMethods();
	}

	AngelScriptEngineInterface::~AngelScriptEngineInterface()
	{
		mEngine = nullptr;

		mScriptEngine->Release();
		mScriptEngine = nullptr;
	}

	void AngelScriptEngineInterface::RegisterGlobalMethods()
	{
		int r = 0;

		r = mScriptEngine->RegisterGlobalFunction("double GetDeltaTime()", asMETHOD(AngelScriptEngineInterface, GetDeltaTime), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = mScriptEngine->RegisterGlobalFunction("double GetFixedTime()", asMETHOD(AngelScriptEngineInterface, GetFixedTime), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);

		// Register component globals
        /*
        r = mScriptEngine->RegisterGlobalFunction("TransformComponent3D@ GetTransform3D(uint64)",
			asMETHOD(AngelScriptEngineInterface, getComponent<TransformComponent3D>), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);

		r = mScriptEngine->RegisterGlobalFunction("bool HasTransform3D(uint64)",
			asMETHOD(AngelScriptEngineInterface, hasComponent<TransformComponent3D>), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
        */

		// Register Input Funcdefs and Bind/Release Callback Methods
		r = mScriptEngine->RegisterFuncdef("void OnInputPressedCallback()"); assert(r >= 0);
		r = mScriptEngine->RegisterFuncdef("void OnInputReleasedCallback()"); assert(r >= 0);

		r = mScriptEngine->RegisterGlobalFunction("void BindOnInputPressed(uint64, const string &in, OnInputPressedCallback @cb)", asMETHOD(AngelScriptEngineInterface, BindOnInputPressed), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = mScriptEngine->RegisterGlobalFunction("void BindOnInputReleased(uint64, const string &in, OnInputReleasedCallback @cb)", asMETHOD(AngelScriptEngineInterface, BindOnInputReleased), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = mScriptEngine->RegisterGlobalFunction("void ReleaseOnInputPressed(uint64, const string &in)", asMETHOD(AngelScriptEngineInterface, ReleaseOnInputPressed), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
		r = mScriptEngine->RegisterGlobalFunction("void ReleaseOnInputReleased(uint64, const string &in)", asMETHOD(AngelScriptEngineInterface, ReleaseOnInputReleased), asCALL_THISCALL_ASGLOBAL, this); assert(r >= 0);
	}

	const double& AngelScriptEngineInterface::GetDeltaTime()
	{
		mDeltaTime = mEngine->GetDeltaTime();
		return mDeltaTime;
	}

	const double& AngelScriptEngineInterface::GetFixedTime()
	{
		mFixedTime = mEngine->GetTimeStepFixed();
		return mFixedTime;
	}

	TransformComponent3D& AngelScriptEngineInterface::GetTransformComponent3D(UUID id) const
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		entt::entity entity = enttSubsystem->GetEntity(id);

		registry->patch<TransformComponent3D>(entity, [&](auto& transform){});

		return registry->get<TransformComponent3D>(entity);
	}

	bool AngelScriptEngineInterface::HasTransformComponent3D(UUID id) const
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		entt::entity entity = enttSubsystem->GetEntity(id);

		if (registry->any_of<TransformComponent3D>(entity))
		{
			return true;
		}

		return false;
	}

	template <typename T>
	T& AngelScriptEngineInterface::getComponent(UUID id) const
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		entt::entity entity = enttSubsystem->GetEntity(id);

		registry->patch<T>(entity, [&](auto& transform) {});

		return registry->get<T>(entity);
	}

	template <typename T>
	bool AngelScriptEngineInterface::HasComponent(UUID id) const
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		entt::entity entity = enttSubsystem->GetEntity(id);

		if (registry->any_of<T>(entity))
		{
			return true;
		}

		return false;
	}

	ScriptCallback AngelScriptEngineInterface::BindCallback(UUID entity, asIScriptFunction* cb) const
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

	void AngelScriptEngineInterface::ReleaseCallback(ScriptCallback& scriptCallback) const
	{
		if (scriptCallback.func)
			scriptCallback.func->Release();

		if (scriptCallback.object)
			mScriptEngine->ReleaseScriptObject(scriptCallback.object, scriptCallback.objectType);

		scriptCallback.func = nullptr;
		scriptCallback.object = nullptr;
		scriptCallback.objectType = nullptr;
	}

	void AngelScriptEngineInterface::BindOnInputPressed(UUID entity, const std::string& actionName,
		asIScriptFunction* cb)
	{
		// Release existing callback function, if one exists
		ReleaseOnInputPressed(entity, actionName);

		mOnInputPressedCallbacks[actionName][entity] = BindCallback(entity, cb);
	}

	void AngelScriptEngineInterface::BindOnInputReleased(UUID entity, const std::string& actionName,
		asIScriptFunction* cb)
	{
		// Release existing callback function, if one exists
		ReleaseOnInputReleased(entity, actionName);

		mOnInputPressedCallbacks[actionName][entity] = BindCallback(entity, cb);
	}

	void AngelScriptEngineInterface::ReleaseOnInputPressed(UUID entity, const std::string& actionName)
	{
		if (mOnInputPressedCallbacks[actionName].count(entity))
		{
			ReleaseCallback(mOnInputPressedCallbacks[actionName][entity]);
			mOnInputPressedCallbacks[actionName].erase(entity);
		}
	}

	void AngelScriptEngineInterface::ReleaseOnInputReleased(UUID entity, const std::string& actionName)
	{
		if (mOnInputPressedCallbacks[actionName].count(entity))
		{
			ReleaseCallback(mOnInputPressedCallbacks[actionName][entity]);
			mOnInputPressedCallbacks[actionName].erase(entity);
		}
	}
}
