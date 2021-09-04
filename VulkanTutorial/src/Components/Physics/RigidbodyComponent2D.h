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

			RigidbodyComponent2D(Float InInvMass) :
				invMass(InInvMass)
			{
			}

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