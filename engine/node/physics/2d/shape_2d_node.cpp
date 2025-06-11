#include "node/physics/2d/shape_2d_node.h"

namespace puffin::physics
{
	const Vector2f& Shape2DNode::GetCentreOfMass() const { return mCentreOfMass; }
	Vector2f& Shape2DNode::CentreOfMass() { return mCentreOfMass; }
	void Shape2DNode::SetCentreOfMass(const Vector2f& centreOfMass) { mCentreOfMass = centreOfMass; }
}
