#include "Scripting/NativeScriptSystem.hpp"

#include "Components/TransformComponent.h"
#include "Components/Scripting/NativeScriptComponent.hpp"

namespace Puffin::Scripting
{
	void NativeScriptSystem::Start()
	{
		std::vector<std::shared_ptr<ECS::Entity>> scriptEntities;
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
		std::vector<std::shared_ptr<ECS::Entity>> scriptEntities;
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
		std::vector<std::shared_ptr<ECS::Entity>> scriptEntities;
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
