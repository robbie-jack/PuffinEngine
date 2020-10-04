#include "TransformSystem.h"

namespace Puffin
{
	void TransformSystem::Init()
	{
		running = true;
		updateWhenPlaying = true;
		Start();
	}

	void TransformSystem::Start()
	{
		transformComponents[0].SetTransform(Puffin::Vector3(2.0f, 0.0f, 0.0f), Puffin::Vector3(0.0f), Puffin::Vector3(1.0f));
		transformComponents[1].SetTransform(Puffin::Vector3(-1.0f, 0.0f, 0.0f), Puffin::Vector3(0.0f), Puffin::Vector3(1.0f));
		transformComponents[2].SetTransform(Puffin::Vector3(0.0f), Puffin::Vector3(0.0f), Puffin::Vector3(1.0f));
		transformComponents[3].SetTransform(Puffin::Vector3(0.0f), Puffin::Vector3(0.0f), Puffin::Vector3(1.0f));
		transformComponents[4].SetTransform(Puffin::Vector3(0.0f, -5.0f, 0.0f), Puffin::Vector3(0.0f), Puffin::Vector3(1.0f));

		for (int i = 0; i < transformComponents.size(); i++)
		{
			Rendering::MeshComponent* meshComp = nullptr;

			// Find entities Mesh component
			for (int j = 0; j < meshComponents->size(); j++)
			{
				if (transformComponents[i].GetEntityID() == meshComponents->at(j).GetEntityID())
				{
					meshComp = &meshComponents->at(j);
					break;
				}
			}

			// Update Mesh component transform from entity transform
			if (meshComp != nullptr)
			{
				meshComp->GetMesh().SetTransform(transformComponents[i].GetTransform());
			}
		}
	}

	bool TransformSystem::Update(float dt)
	{
		for (int i = 0; i < transformComponents.size(); i++)
		{
			Physics::ReactPhysicsComponent* physicsComp = nullptr;
			Rendering::MeshComponent* meshComp = nullptr;

			// Find entities Mesh component
			for (int j = 0; j < meshComponents->size(); j++)
			{
				if (transformComponents[i].GetEntityID() == meshComponents->at(j).GetEntityID())
				{
					meshComp = &meshComponents->at(j);
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

				transformComponents[i].SetPosition(physicsTransform.getPosition());
				transformComponents[i].SetRotation(ConvertToEulerAngles(physicsTransform.getOrientation()));
			}

			// Update Mesh component transform from entity transform
			if (meshComp != nullptr)
			{
				meshComp->GetMesh().SetTransform(transformComponents[i].GetTransform());
			}
		}

		return true;
	}

	void TransformSystem::Stop()
	{
		Start();
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
}