#include "node/physics/2d/box_2d_node.h"

namespace puffin::physics
{
	void Box2DNode::Serialize(nlohmann::json& json) const
	{
		Shape2DNode::Serialize(json);

		nlohmann::json boxJson;
		boxJson["centreOfMass"] = serialization::Serialize(GetCentreOfMass());
		boxJson["halfExtent"] = serialization::Serialize(mHalfExtent);

		json["box"] = boxJson;
	}

	void Box2DNode::Deserialize(const nlohmann::json& json)
	{
		Shape2DNode::Deserialize(json);

		auto& boxJson = json["box"];

		SetCentreOfMass(serialization::Deserialize<Vector2f>(boxJson["centreOfMass"]));
		mHalfExtent = serialization::Deserialize<Vector2f>(boxJson["halfExtent"]);
	}

	std::string_view Box2DNode::GetTypeString() const
	{
		return reflection::GetTypeString<Box2DNode>();
	}

	entt::id_type Box2DNode::GetTypeID() const
	{
		return reflection::GetTypeHashedString<Box2DNode>();
	}

	const Vector2f& Box2DNode::GetHalfExtent() const { return mHalfExtent; }
	Vector2f& Box2DNode::HalfExtent() { return mHalfExtent; }
	void Box2DNode::SetHalfExtent(const Vector2f& halfExtent) { mHalfExtent = halfExtent; }
}
