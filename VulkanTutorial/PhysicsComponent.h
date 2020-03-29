#pragma once

#include "BaseComponent.h"
#include "BulletCollision/CollisionShapes/btCollisionShape.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"

class PhysicsComponent : public BaseComponent
{
public:

	PhysicsComponent();
	~PhysicsComponent();

	inline btTransform GetTransform() { return transform; };
	inline void SetTransform(btTransform transform_) { transform = transform_; };

	inline btRigidBody* GetRigidBody() { return rigidbody; };
	inline void SetRigidBody(btRigidBody* rigidbody_) { rigidbody = rigidbody_; };

	inline btCollisionShape* GetShape() { return collisionShape; };
	inline void SetShape(btCollisionShape* shape) { collisionShape = shape; };

protected:

	btTransform transform;
	btRigidBody* rigidbody;
	btCollisionShape* collisionShape;
};