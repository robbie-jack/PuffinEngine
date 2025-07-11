#pragma once

#include <memory>

#include "component/physics/2d/rigidbody_component_2d.h"
#include "ecs/entt_subsystem.h"
#include "physics/onager2d/colliders/collider_2d.h"
#include "physics/onager2d/physics_helpers_2d.h"
#include "types/storage/mapped_vector.h"

namespace puffin::physics
{
	typedef std::pair<const std::shared_ptr<collision2D::Collider2D>, const std::shared_ptr<collision2D::Collider2D>> CollisionPair;

	class Broadphase
	{
	public:

		virtual ~Broadphase() = default;

		virtual void generateCollisionPairs(MappedVector<UUID, std::shared_ptr<collision2D::Collider2D>>& colliders, std::vector<CollisionPair>& collisionPairs, bool
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

			const auto registry = mEcs->GetRegistry();

			if (!registry->valid(mEcs->GetEntity(pair.first->uuid)) || !registry->valid(mEcs->GetEntity(pair.second->uuid)))
				return false;

			// Don't perform collision check between colliders which both have infinite mass
			const auto& rbA = registry->get<RigidbodyComponent2D>(mEcs->GetEntity(pair.first->uuid));
			const auto& rbB = registry->get<RigidbodyComponent2D>(mEcs->GetEntity(pair.second->uuid));

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

		void generateCollisionPairs(MappedVector<UUID, std::shared_ptr<collision2D::Collider2D>>& colliders, std::vector<CollisionPair>& collisionPairs, bool
		                            collidersUpdated) override
		{
			collisionPairs.clear();
			collisionPairs.reserve(colliders.Size() * colliders.Size());

			for (const auto colliderA : colliders)
			{
				for (const auto colliderB : colliders)
				{
					auto collisionPair = std::make_pair(colliderA, colliderB);

					if (filterCollisionPair(collisionPair, collisionPairs) == true)
					{
						AABB2D a = colliderA->getAABB();
						AABB2D b = colliderB->getAABB();

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
