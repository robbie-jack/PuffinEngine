#pragma once

#include "angelscript.h"

#include "ECS/ECS.h"

#include "Components/TransformComponent.h"

typedef Puffin::TransformComponent STransformComponent;

namespace Puffin::Scripting
{
	/*
	* Example Code for Constructor/Deconstructor
	* Replace "Object" with the name of the type you are Implementing
	static void ConstructObject(void* memory)
	{
		new(memory) Object();
	}

	static void DeconstructVector3f(void* memory)
	{
		((Object*)memory)->~Object();
	}
	*/

	/*
	* Vector3f
	*/
	static void ConstructVector3f(void* memory)
	{
		new(memory) Vector3f();
	}

	static void DeconstructVector3f(void* memory)
	{
		((Vector3f*)memory)->~Vector3f();
	}

	static void RegisterVector3f(asIScriptEngine* scriptEngine)
	{
		int r;

		r = scriptEngine->RegisterObjectType("Vector3f", sizeof(Vector3f), asOBJ_VALUE | asGetTypeTraits<Vector3f>()); assert(r >= 0);

		r = scriptEngine->RegisterObjectBehaviour("Vector3f", asBEHAVE_CONSTRUCT, "void Vector3f()", asFUNCTION(ConstructVector3f), asCALL_CDECL_OBJLAST); assert(r >= 0);
		r = scriptEngine->RegisterObjectBehaviour("Vector3f", asBEHAVE_DESTRUCT, "void Vector3f()", asFUNCTION(DeconstructVector3f), asCALL_CDECL_OBJLAST); assert(r >= 0);

		r = scriptEngine->RegisterObjectProperty("Vector3f", "float x", asOFFSET(Vector3f, x)); assert(r >= 0);
		r = scriptEngine->RegisterObjectProperty("Vector3f", "float y", asOFFSET(Vector3f, y)); assert(r >= 0);
		r = scriptEngine->RegisterObjectProperty("Vector3f", "float z", asOFFSET(Vector3f, z)); assert(r >= 0);
	}

	/*
	* Vector3d
	*/
	static void ConstructVector3d(void* memory)
	{
		new(memory) Vector3d();
	}

	static void DeconstructVector3d(void* memory)
	{
		((Vector3d*)memory)->~Vector3d();
	}

	static void RegisterVector3d(asIScriptEngine* scriptEngine)
	{
		int r;

		r = scriptEngine->RegisterObjectType("Vector3d", sizeof(Vector3d), asOBJ_VALUE | asGetTypeTraits<Vector3d>()); assert(r >= 0);
		r = scriptEngine->RegisterObjectBehaviour("Vector3d", asBEHAVE_CONSTRUCT, "void Vector3d()", asFUNCTION(ConstructVector3d), asCALL_CDECL_OBJLAST); assert(r >= 0);
		r = scriptEngine->RegisterObjectBehaviour("Vector3d", asBEHAVE_DESTRUCT, "void Vector3d()", asFUNCTION(DeconstructVector3d), asCALL_CDECL_OBJLAST); assert(r >= 0);

		r = scriptEngine->RegisterObjectProperty("Vector3d", "double x", asOFFSET(Vector3d, x)); assert(r >= 0);
		r = scriptEngine->RegisterObjectProperty("Vector3d", "double y", asOFFSET(Vector3d, y)); assert(r >= 0);
		r = scriptEngine->RegisterObjectProperty("Vector3d", "double z", asOFFSET(Vector3d, z)); assert(r >= 0);
	}

	/*
	* TransformComponent
	*/
	static void DeconstructTransformComponent(TransformComponent* thisPointer)
	{
		((TransformComponent*)thisPointer)->~TransformComponent();
	}

	static STransformComponent* TransformComponentFactory()
	{
		return new STransformComponent();
	}

	static void RegisterTransformComponent(asIScriptEngine* scriptEngine, ECS::World* world)
	{
		int r;

		// Register Required Types
		RegisterVector3f(scriptEngine);
		RegisterVector3d(scriptEngine);

		// Register Class Constructor/Deconstructor
		r = scriptEngine->RegisterObjectType("STransformComponent", sizeof(STransformComponent), asOBJ_REF | asOBJ_NOCOUNT); assert(r >= 0);
		r = scriptEngine->RegisterObjectBehaviour("STransformComponent", asBEHAVE_FACTORY, "STransformComponent@ f()", asFUNCTION(TransformComponentFactory), asCALL_CDECL); assert(r >= 0);

		// Register Class Properties
#ifdef PFN_USE_DOUBLE_PRECISION
		r = scriptEngine->RegisterObjectProperty("STransformComponent", "Vector3d position", asOFFSET(TransformComponent, position)); assert(r >= 0);
#else
		r = scriptEngine->RegisterObjectProperty("STransformComponent", "Vector3f position", asOFFSET(TransformComponent, position)); assert(r >= 0);
#endif

		r = scriptEngine->RegisterObjectProperty("STransformComponent", "Vector3f rotation", asOFFSET(TransformComponent, rotation)); assert(r >= 0);
		r = scriptEngine->RegisterObjectProperty("STransformComponent", "Vector3f scale", asOFFSET(TransformComponent, scale)); assert(r >= 0);

		// Register Operator Overloads
		r = scriptEngine->RegisterObjectMethod("STransformComponent", "STransformComponent& opAssign(const STransformComponent &in)", asMETHODPR(STransformComponent, operator=, (const STransformComponent&), STransformComponent&), asCALL_THISCALL);

		// Register ECS World Access Functions
		r = scriptEngine->RegisterGlobalFunction("STransformComponent@ GetTransformComponent(uint)",
			asMETHOD(ECS::World, GetComponent<TransformComponent>), asCALL_THISCALL_ASGLOBAL, world); assert(r >= 0);

		r = scriptEngine->RegisterGlobalFunction("bool HasTransformComponent(uint)",
			asMETHOD(ECS::World, HasComponent<TransformComponent>), asCALL_THISCALL_ASGLOBAL, world); assert(r >= 0);
	}
}

