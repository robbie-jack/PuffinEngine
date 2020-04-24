#include "TransformSystem.h"

void TransformSystem::Init()
{

}

bool TransformSystem::Update(float dt)
{
	for (int i = 0; i < transformComponents.size(); i++)
	{
		ReactPhysicsComponent* physicsComp = nullptr;
		RenderComponent* renderComp = nullptr;

		// Find entities render component
		for (int j = 0; j < renderComponents->size(); j++)
		{
			if (transformComponents[i].GetEntityID() == renderComponents->at(j).GetEntityID())
			{
				renderComp = &renderComponents->at(j);
				break;
			}
		}

		// Find entities physics component
		for (int j = 0; j < physicsComponents->size(); j++)
		{
			if (transformComponents[i].GetEntityID() == physicsComponents->at(j).GetEntityID())
			{
				physicsComp = &physicsComponents->at(j);
				break;
			}
		}

		// Update Entity Transform with physics component position and rotation
		if (physicsComp != nullptr)
		{
			rp3d::Transform physicsTransform = physicsComp->GetLerpTransform();

			transformComponents[i].SetPosition(ConvertVectorToGLM(physicsTransform.getPosition()));
			transformComponents[i].SetRotation(ConvertVectorToGLM(ConvertToEulerAngles(physicsTransform.getOrientation())));
		}

		// Update render component transform from entity transform
		if (renderComp != nullptr)
		{
			renderComp->GetMesh().SetTransform(transformComponents[i].GetTransform());
		}
	}

	return true;
}

void TransformSystem::SendMessage()
{

}

TransformComponent* TransformSystem::AddComponent()
{
	TransformComponent transformComponent;
	transformComponents.push_back(transformComponent);
	return &transformComponents.back();
}

TransformComponent* TransformSystem::GetComponent(uint32_t entityID)
{
	for (auto comp : transformComponents)
	{
		if (comp.GetEntityID() == entityID)
		{
			return &comp;
		}
	}
}

TransformSystem::~TransformSystem()
{
	transformComponents.clear();
}