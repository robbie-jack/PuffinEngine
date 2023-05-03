#pragma once

#include "ECS/ECS.h"
#include "Components/Physics/RigidbodyComponent2D.h"
#include "Physics/Onager2D/Colliders/Collider2D.h"
#include "Types/PackedArray.h"
#include "Physics/Onager2D/PhysicsHelpers2D.h"
#include "ECS/EnTTSubsystem.hpp"

#include <memory>

namespace Puffin::Physics
{
	typedef std::pair<const std::shared_ptr<Collision2D::Collider2D>, const std::shared_ptr<Collision2D::Collider2D>> CollisionPair;

	class Broadphase
	{
	public:

		virtual ~Broadphase() = default;

		virtual void GenerateCollisionPairs(PackedVector<std::shared_ptr<Collision2D::Collider2D>>& inColliders, std::vector<CollisionPair>& outCollisionPairs, bool
		                                    collidersUpdated) = 0;

		void SetWorld(std::shared_ptr<ECS::World> world)
		{
			m_world = world;
		}

		void SetECS(std::shared_ptr<ECS::EnTTSubsystem> ecs)
		{
			m_ecs = ecs;
		}

	protected:

		bool FilterCollisionPair(const CollisionPair& pair, const std::vector<CollisionPair>& collisionPairs) const
		{
			// Don't perform collision check between collider and itself
			if (pair.first->uuid == pair.second->uuid)
				return false;

			auto registry = m_ecs->Registry();

			if (!registry->valid(m_ecs->GetEntity(pair.first->uuid)) || !registry->valid(m_ecs->GetEntity(pair.second->uuid)))
				return false;

			// Don't perform collision check between colliders which both have infinite mass
			const auto& rbA = registry->get<RigidbodyComponent2D>(m_ecs->GetEntity(pair.first->uuid));
			const auto& rbB = registry->get<RigidbodyComponent2D>(m_ecs->GetEntity(pair.second->uuid));

			if (rbA.mass == 0.0f && rbB.mass == 0.0f)
				return false;

			// Don't perform collision check if this pair is already in list
			bool collisionPairAlreadyExists = false;

			for (const CollisionPair& collisionPair : collisionPairs)
			{
				collisionPairAlreadyExists |= collisionPair.first->uuid == pair.second->uuid && collisionPair.second->uuid == pair.first->uuid;
			}

			if (collisionPairAlreadyExists)
				return false;

			return true;
		}

		std::shared_ptr<ECS::World> m_world = nullptr;
		std::shared_ptr<ECS::EnTTSubsystem> m_ecs = nullptr;

	};

	class NSquaredBroadphase : public Broadphase
	{
	public:

		NSquaredBroadphase() = default;
		~NSquaredBroadphase() override = default;

		void GenerateCollisionPairs(PackedVector<std::shared_ptr<Collision2D::Collider2D>>& inColliders, std::vector<CollisionPair>& outCollisionPairs, bool
		                            collidersUpdated) override
		{
			outCollisionPairs.clear();
			outCollisionPairs.reserve(inColliders.Size() * inColliders.Size());

			for (const auto colliderA : inColliders)
			{
				for (const auto colliderB : inColliders)
				{
					auto collisionPair = std::make_pair(colliderA, colliderB);

					if (FilterCollisionPair(collisionPair, outCollisionPairs) == true)
					{
						AABB a = colliderA->GetAABB();
						AABB b = colliderB->GetAABB();

						if (Collision2D::TestAABBVsAABB(a, b))
						{
							outCollisionPairs.emplace_back(collisionPair);
						}
					}
				}
			}
		}
	};
}
