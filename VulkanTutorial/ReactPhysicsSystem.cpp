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
			settings.isSleepingEnabled = false;
			settings.gravity = rp3d::Vector3(0.0f, -9.8f, 0.0f);

			// Initialize Physics World
			physicsWorld = physicsCommon.createPhysicsWorld(settings);

			timeSinceLastUpdate = 0.0f;

			InitComponent(3);
			InitComponent(5, rp3d::Vector3(0.0f, -5.0f, 0.0f), rp3d::Vector3(0.0f, 0.0f, 0.0f), rp3d::BodyType::STATIC);
		}

		bool ReactPhysicsSystem::Update(float dt)
		{
			// Add Frame Time onto timeSinceLastUpdate value
			timeSinceLastUpdate += dt;

			// Update Physics World when timeStep is exceeded
			if (timeSinceLastUpdate >= timeStep)
			{
				// Update Physics world with fixed time step
				physicsWorld->update(timeStep);

				// Decrease time since last update
				timeSinceLastUpdate -= timeStep;
			}

			rp3d::decimal factor = timeSinceLastUpdate / timeStep;

			for (ECS::Entity entity : entities)
			{
				ReactPhysicsComponent& comp = world->GetComponent<ReactPhysicsComponent>(entity);

				// Get current transform from dynamics world
				rp3d::Transform currTransform = comp.body->getTransform();

				// Calculate Interpolated Transform between previous and current transforms
				rp3d::Transform lerpTransform = rp3d::Transform::interpolateTransforms(comp.prevTransform, currTransform, factor);

				// Set this entities transform to lerp transform
				TransformComponent& transformComp = world->GetComponent<TransformComponent>(entity);
				transformComp.position = lerpTransform.getPosition();
				transformComp.rotation = ConvertToEulerAngles(lerpTransform.getOrientation());

				// Set previous transform to current transform for this frame
				comp.prevTransform = currTransform;
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

		void ReactPhysicsSystem::InitComponent(ECS::Entity entity, rp3d::Vector3 position, rp3d::Vector3 rotation, rp3d::BodyType bodyType)
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