#pragma once

#ifndef RIGIDBODY_COMPONENT_2D_H
#define RIGIDBODY_COMPONENT_2D_H

namespace Puffin
{
	namespace Physics
	{
		struct RigidbodyComponent2D
		{
			RigidbodyComponent2D() {}

			RigidbodyComponent2D(float InInvMass) :
				invMass(InInvMass)
			{
			}

			Vector2f linearVelocity = Vector2f(0.0f);
			float angularVelocity = 0.0f;

			float invMass = 0.0f;
			float elasticity = 1.0f;
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