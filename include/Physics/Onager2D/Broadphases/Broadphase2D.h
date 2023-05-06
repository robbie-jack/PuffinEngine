#pragma once

#include "Components/Physics/RigidbodyComponent2D.h"
#include "ECS/EnTTSubsystem.h"
#include "Physics/Onager2D/PhysicsHelpers2D.h"
#include "Physics/Onager2D/Colliders/Collider2D.h"
#include "Types/PackedArray.h"

#include <memory>

namespace puffin::physics
{
	typedef std::pair<const std::shared_ptr<collision2D::Collider2D>, const std::shared_ptr<collision2D::Collider2D>> CollisionPair;

	class Broadphase
	{
	public:

		virtual ~Broadphase() = default;

		virtual void generateCollisionPairs(PackedVector<std::shared_ptr<collision2D::Collider2D>>& colliders, std::vector<CollisionPair>& collisionPairs, bool
		                                    collidersUpdated) = 0;

		void setECS(const std::shared_ptr<ECS::EnTTSubsystem>& ecs)
		{
			mEcs = ecs;
		}

	protected:

		bool filterCollisionPair(const CollisionPair& pair, const std::vector<CollisionPair>& collisionPairs) const
		{
			// Don't perform collision check between collider and itself
			if (pair.first->uuid == pair.second->uuid)
				return false;

			const auto registry = mEcs->registry();

			if (!registry->valid(mEcs->getEntity(pair.first->uuid)) || !registry->valid(mEcs->getEntity(pair.second->uuid)))
				return false;

			// Don't perform collision check between colliders which both have infinite mass
			const auto& rbA = registry->get<RigidbodyComponent2D>(mEcs->getEntity(pair.first->uuid));
			const auto& rbB = registry->get<RigidbodyComponent2D>(mEcs->getEntity(pair.second->uuid));

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

		std::shared_ptr<ECS::EnTTSubsystem> mEcs = nullptr;

	};

	class NSquaredBroadphase : public Broadphase
	{
	public:

		NSquaredBroadphase() = default;
		~NSquaredBroadphase() override = default;

		void generateCollisionPairs(PackedVector<std::shared_ptr<collision2D::Collider2D>>& colliders, std::vector<CollisionPair>& collisionPairs, bool
		                            collidersUpdated) override
		{
			collisionPairs.clear();
			collisionPairs.reserve(colliders.size() * colliders.size());

			for (const auto colliderA : colliders)
			{
				for (const auto colliderB : colliders)
				{
					auto collisionPair = std::make_pair(colliderA, colliderB);

					if (filterCollisionPair(collisionPair, collisionPairs) == true)
					{
						AABB a = colliderA->getAABB();
						AABB b = colliderB->getAABB();

						if (collision2D::testAabbVsAabb(a, b))
						{
							collisionPairs.emplace_back(collisionPair);
						}
					}
				}
			}
		}
	};
}
