#include "node/physics/2d/shape_2d_node.h"

namespace puffin::physics
{
	std::string_view Shape2DNode::GetTypeString() const
	{
		return reflection::GetTypeString<Shape2DNode>();
	}

	entt::id_type Shape2DNode::GetTypeID() const
	{
		return reflection::GetTypeHashedString<Shape2DNode>();
	}

	const Vector2f& Shape2DNode::GetCentreOfMass() const { return mCentreOfMass; }
	Vector2f& Shape2DNode::CentreOfMass() { return mCentreOfMass; }
	void Shape2DNode::SetCentreOfMass(const Vector2f& centreOfMass) { mCentreOfMass = centreOfMass; }
}
