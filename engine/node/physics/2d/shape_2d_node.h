#pragma once

#include "node/transform_2d_node.h"

namespace puffin::physics
{
	class Shape2DNode : public Transform2DNode
	{
	public:

		const Vector2f& GetCentreOfMass() const;
		Vector2f& CentreOfMass();
		void SetCentreOfMass(const Vector2f& centreOfMass);

	private:

		Vector2f mCentreOfMass = { 0.0f };

	};
}