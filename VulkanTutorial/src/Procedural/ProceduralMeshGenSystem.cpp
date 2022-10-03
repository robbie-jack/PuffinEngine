#include "Procedural/ProceduralMeshGenSystem.hpp"

#include "Engine/Engine.hpp"
#include "ECS/Entity.h"

#include "Components/TransformComponent.h"
#include "Components/Procedural/ProceduralComponent.hpp"

namespace Puffin::Procedural
{
	void ProceduralMeshGenSystem::Update()
	{
		std::vector<std::shared_ptr<ECS::Entity>> proceduralPlaneEntities;
		ECS::GetEntities<TransformComponent, ProceduralPlaneComponent>(m_world, proceduralPlaneEntities);
		for (const auto& entity : proceduralPlaneEntities)
		{
			if (entity->GetComponentFlag<ProceduralPlaneComponent, FlagDirty>())
			{
				
			}

			if (entity->GetComponentFlag<ProceduralPlaneComponent, FlagDeleted>())
			{

			}
		}
	}
}
