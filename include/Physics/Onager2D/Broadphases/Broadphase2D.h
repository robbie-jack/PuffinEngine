#pragma once

#include "Physics/Onager2D/PhysicsHelpers2D.h"
#include "Physics/Onager2D/Colliders/Collider2D.h"
#include "puffin/types/packed_array.h"

#include <memory>

#include "puffin/components/physics/2d/rigidbody_component_2d.h"
#include "puffin/ecs/entt_subsystem.h"

namespace puffin::physics
{
	typedef std::pair<const std::shared_ptr<collision2D::Collider2D>, const std::shared_ptr<collision2D::Collider2D>> CollisionPair;

	class Broadphase
	{
	public:

		virtual ~Broadphase() = default;

		virtual void generateCollisionPairs(PackedVector<std::shared_ptr<collision2D::Collider2D>>& colliders, std::vector<CollisionPair>& collisionPairs, bool
		                                    collidersUpdated) = 0;

		void setECS(const std::shared_ptr<ecs::EnTTSubsystem>& ecs)
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

			if (!registry->valid(mEcs->get_entity(pair.first->uuid)) || !registry->valid(mEcs->get_entity(pair.second->uuid)))
				return false;

			// Don't perform collision check between colliders which both have infinite mass
			const auto& rbA = registry->get<RigidbodyComponent2D>(mEcs->get_entity(pair.first->uuid));
			const auto& rbB = registry->get<RigidbodyComponent2D>(mEcs->get_entity(pair.second->uuid));

			if (rbA.mass == 0.0f && rbB.mass == 0.0f)
				return false;

			// Don't perform collision check if this pair is already in list
			for (const CollisionPair& collisionPair : collisionPairs)
			{
				if (pair.first == collisionPair.first && pair.second == collisionPair.second)
					return false;

				if (pair.second == collisionPair.first && pair.first == collisionPair.second)
					return false;
			}

			return true;
		}

		std::shared_ptr<ecs::EnTTSubsystem> mEcs = nullptr;

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
