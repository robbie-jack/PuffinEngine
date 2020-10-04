#pragma once

#include "System.h"
#include "TransformComponent.h"
#include "MeshComponent.h"
#include "ReactPhysicsComponent.h"

#include "VectorConversion.h"

#include <vector>

namespace Puffin
{
	class TransformSystem : public System
	{
	public:

		void Init();
		void Start();
		bool Update(float dt);
		void Stop();
		void SendMessage();

		TransformComponent* AddComponent();
		TransformComponent* GetComponent(uint32_t entityID);

		inline void SetPhysicsRenderVectors(std::vector<Physics::ReactPhysicsComponent>* physicsComponents_, std::vector<Rendering::MeshComponent>* meshComponents_)
		{
			physicsComponents = physicsComponents_;
			meshComponents = meshComponents_;
		}

		~TransformSystem();

	private:

		std::vector<TransformComponent> transformComponents;
		std::vector<Physics::ReactPhysicsComponent>* physicsComponents;
		std::vector<Rendering::MeshComponent>* meshComponents;
	};
}