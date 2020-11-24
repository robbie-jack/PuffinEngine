#include "ReactPhysicsSystem.h"
#include "TransformComponent.h"
#include "VectorConversion.h"

#include <cmath>

namespace Puffin
{
	namespace Physics
	{
		void ReactPhysicsSystem::Start()
		{
			// Define Physics World Settings
			rp3d::PhysicsWorld::WorldSettings settings;
			settings.defaultPositionSolverNbIterations = 20;
			settings.defaultVelocitySolverNbIterations = 8;
			settings.isSleepingEnabled = true;
			settings.gravity = rp3d::Vector3(0.0f, -9.8f, 0.0f);

			// Initialize Physics World
			physicsWorld = physicsCommon.createPhysicsWorld(settings);

			accumulatedTime = 0.0f;

			TransformComponent& comp1 = world->GetComponent<TransformComponent>(3);
			TransformComponent& comp2 = world->GetComponent<TransformComponent>(5);

			InitComponent(3, rp3d::BodyType::DYNAMIC, comp1.position);
			InitComponent(5, rp3d::BodyType::KINEMATIC, comp2.position);
		}

		bool ReactPhysicsSystem::Update(float dt)
		{
			// Add Frame Time onto timeSinceLastUpdate value
			accumulatedTime += dt;

			// Update Physics World while timeStep is exceeded
			while (accumulatedTime >= timeStep)
			{
				// Update Physics world with fixed time step
				physicsWorld->update(timeStep);

				// Decrease time since last update
				accumulatedTime -= timeStep;
			}

			rp3d::decimal factor = accumulatedTime / timeStep;

			for (ECS::Entity entity : entities)
			{
				TransformComponent& transformComp = world->GetComponent<TransformComponent>(entity);
				ReactPhysicsComponent& comp = world->GetComponent<ReactPhysicsComponent>(entity);

				// Initialise any new components
				if (comp.flag_created)
				{
					InitComponent(entity, BodyType::STATIC, transformComp.position);
					comp.flag_created = false;
				}

				// Re-initialse components
				/*if (comp.flag_recreate)
				{

				}*/

				// Get current transform from dynamics world
				rp3d::Transform currTransform = comp.body->getTransform();

				// Calculate Interpolated Transform between previous and current transforms
				rp3d::Transform lerpTransform = rp3d::Transform::interpolateTransforms(comp.prevTransform, currTransform, factor);

				// Set this entities transform to lerp transform
				transformComp.position = lerpTransform.getPosition();
				transformComp.rotation = ConvertToEulerAngles(lerpTransform.getOrientation());

				// Set previous transform to current transform for this frame
				comp.prevTransform = currTransform;

				// Delete marked components
				if (comp.flag_deleted || world->IsDeleted(entity))
				{
					physicsWorld->destroyRigidBody(comp.body);
					world->RemoveComponent<ReactPhysicsComponent>(entity);
				}
			}

			return true;
		}

		void ReactPhysicsSystem::Stop()
		{
			// Destroy All Rigid Bodies
			for (ECS::Entity entity : entities)
			{
				ReactPhysicsComponent& comp = world->GetComponent<ReactPhysicsComponent>(entity);

				physicsWorld->destroyRigidBody(comp.body);
			}

			// Destroy Dynamics World
			physicsCommon.destroyPhysicsWorld(physicsWorld);
			//physicsWorld = NULL;
		}

		void ReactPhysicsSystem::InitComponent(ECS::Entity entity, rp3d::BodyType bodyType, rp3d::Vector3 position, rp3d::Vector3 rotation)
		{
			ReactPhysicsComponent& comp = world->GetComponent<ReactPhysicsComponent>(entity);

			// Create Quaternion/Transform of new rigid body
			rp3d::Quaternion initQuaternion = rp3d::Quaternion::fromEulerAngles(rotation);
			rp3d::Transform transform(position, initQuaternion);

			// Create Rigid Body with dynamics world and store in component
			comp.body = physicsWorld->createRigidBody(transform);
			comp.prevTransform = transform;
			comp.body->setType(bodyType);
		}

		ReactPhysicsSystem::~ReactPhysicsSystem()
		{
			// Clear Components Vector
			Stop();
			entities.clear();
			//world.reset();
		}
	}
}