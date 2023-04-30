#include "Scripting/NativeScriptSystem.hpp"

#include "Components/TransformComponent.h"
#include "Components/Scripting/NativeScriptComponent.hpp"

namespace Puffin::Scripting
{
	void NativeScriptSystem::Start()
	{
		PackedVector<ECS::EntityPtr> scriptEntities;
		ECS::GetEntities<TransformComponent, NativeScriptComponent>(m_world, scriptEntities);
		for (const auto entity : scriptEntities)
		{
			auto& script = entity->GetComponent<NativeScriptComponent>();

			if (script.object != nullptr)
			{
				script.object->Start();
			}
		}
	}

	void NativeScriptSystem::Update()
	{
		PackedVector<ECS::EntityPtr> scriptEntities;
		ECS::GetEntities<TransformComponent, NativeScriptComponent>(m_world, scriptEntities);
		for (const auto entity : scriptEntities)
		{
			auto& script = entity->GetComponent<NativeScriptComponent>();

			if (script.object != nullptr)
			{
				script.object->Update();
			}
		}
	}

	void NativeScriptSystem::Stop()
	{
		PackedVector<ECS::EntityPtr> scriptEntities;
		ECS::GetEntities<TransformComponent, NativeScriptComponent>(m_world, scriptEntities);
		for (const auto entity : scriptEntities)
		{
			auto& script = entity->GetComponent<NativeScriptComponent>();

			if (script.object != nullptr)
			{
				script.object->Stop();
			}
		}
	}
}
