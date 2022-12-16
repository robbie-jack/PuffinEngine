#pragma once

#include "ECS/ECS.h"
#include "Components/Physics/RigidbodyComponent2D.h"
#include "Physics/Onager2D/Colliders/Collider2D.h"

#include "Types/PackedArray.h"

#include <memory>

namespace Puffin::Physics
{
	typedef std::pair<const std::shared_ptr<Collision2D::Collider2D>, const std::shared_ptr<Collision2D::Collider2D>> CollisionPair;

	class Broadphase
	{
	public:

		virtual ~Broadphase() = default;

		virtual void GenerateCollisionPairs(PackedVector<std::shared_ptr<Collision2D::Collider2D>>& inColliders, std::vector<CollisionPair>& outCollisionPairs) = 0;

		void SetWorld(std::shared_ptr<ECS::World> world)
		{
			m_world = world;
		}

	protected:

		bool FilterCollisionPair(const CollisionPair& pair, const std::vector<CollisionPair>& collisionPairs) const
		{
			// Don't perform collision check between collider and itself
			if (pair.first->entity == pair.second->entity)
				return false;

			// Don't perform collision check between colliders which both have infinite mass
			const auto& rbA = m_world->GetComponent<RigidbodyComponent2D>(pair.first->entity);
			const auto& rbB = m_world->GetComponent<RigidbodyComponent2D>(pair.second->entity);

			if (rbA.invMass == 0.0f && rbB.invMass == 0.0f)
				return false;

			// Don't perform collision check if this pair is already in list
			bool collisionPairAlreadyExists = false;

			for (const CollisionPair& collisionPair : collisionPairs)
			{
				collisionPairAlreadyExists |= collisionPair.first->entity == pair.second->entity && collisionPair.second->entity == pair.first->entity;
			}

			if (collisionPairAlreadyExists)
				return false;

			return true;
		}

		std::shared_ptr<ECS::World> m_world = nullptr;

	};

	class NSquaredBroadphase : public Broadphase
	{
	public:

		NSquaredBroadphase() = default;
		~NSquaredBroadphase() override = default;

		void GenerateCollisionPairs(PackedVector<std::shared_ptr<Collision2D::Collider2D>>& inColliders, std::vector<CollisionPair>& outCollisionPairs) override
		{
			outCollisionPairs.clear();
			outCollisionPairs.reserve(inColliders.Size() * inColliders.Size());

			for (const auto colliderA : inColliders)
			{
				for (const auto colliderB : inColliders)
				{
					auto collisionPair = std::make_pair(colliderA, colliderB);

					if (FilterCollisionPair(collisionPair, outCollisionPairs) == true)
						outCollisionPairs.push_back(collisionPair);
				}
			}
		}
	};
}