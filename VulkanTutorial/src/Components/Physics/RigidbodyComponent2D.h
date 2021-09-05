#pragma once

#ifndef RIGIDBODY_COMPONENT_2D_H
#define RIGIDBODY_COMPONENT_2D_H

#include <Types/PhysicsTypes2D.h>

namespace Puffin
{
	namespace Physics
	{
		struct RigidbodyComponent2D
		{
			RigidbodyComponent2D() {}

			RigidbodyComponent2D(Float InInvMass) :
				invMass(InInvMass)
			{
				shapeType = Collision2D::ShapeType::NONE;
			}

			Collision2D::ShapeType shapeType; // Type of shape attached to same entity, if any

			Vector2 linearVelocity = Vector2(0.0f);
			Float angularVelocity = 0.0f;

			Float invMass = 0.0f;
			Float elasticity = 1.0f;
		};

		template<class Archive>
		void serialize(Archive& archive, RigidbodyComponent2D& body)
		{
			archive(body.invMass);
			archive(body.elasticity);
		}
	}
}

#endif //RIGIDBODY_COMPONENT_2D_H