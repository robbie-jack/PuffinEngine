#include "node/physics/2d/rigidbody_2d_node.h"

namespace puffin::physics
{
	void Rigidbody2DNode::Serialize(nlohmann::json& json) const
	{
		Transform2DNode::Serialize(json);

		nlohmann::json rigidbodyJson;
		rigidbodyJson["bodyType"] = gBodyTypeToString.at(mBodyType);
		rigidbodyJson["mass"] = mMass;
		rigidbodyJson["density"] = mDensity;
		rigidbodyJson["elasticity"] = mElasticity;
		rigidbodyJson["friction"] = mFriction;
		rigidbodyJson["linearVelocity"] = serialization::Serialize(mLinearVelocity);
		rigidbodyJson["angularVelocity"] = mAngularVelocity;

		json["rigidbody"] = rigidbodyJson;
	}

	void Rigidbody2DNode::Deserialize(const nlohmann::json& json)
	{
		Transform2DNode::Deserialize(json);

		auto& rigidbodyJson = json["rigidbody"];

		mBodyType = gStringToBodyType.at(rigidbodyJson["bodyType"]);
		mMass = rigidbodyJson["mass"];
		mDensity = rigidbodyJson["density"];
		mElasticity = rigidbodyJson["elasticity"];
		mFriction = rigidbodyJson["friction"];
		mLinearVelocity = serialization::Deserialize<Vector2f>(rigidbodyJson["linearVelocity"]);
		mAngularVelocity = rigidbodyJson["angularVelocity"];
	}

	std::string_view Rigidbody2DNode::GetTypeString() const
	{
		return reflection::GetTypeString<Rigidbody2DNode>();
	}

	entt::id_type Rigidbody2DNode::GetTypeID() const
	{
		return reflection::GetTypeHashedString<Rigidbody2DNode>();
	}

	BodyType Rigidbody2DNode::GetBodyType() const { return mBodyType; }
	BodyType& Rigidbody2DNode::BodyType() { return mBodyType; }
	void Rigidbody2DNode::SetBodyType(physics::BodyType bodyType) { mBodyType = bodyType; }

	float Rigidbody2DNode::GetMass() const { return mMass; }
	float& Rigidbody2DNode::Mass() { return mMass; }
	void Rigidbody2DNode::SetMass(float mass) { mMass = mass; }

	float Rigidbody2DNode::GetDensity() const { return mDensity; }
	float& Rigidbody2DNode::Density() { return mDensity; }
	void Rigidbody2DNode::SetDensity(float density) { mDensity = density; }

	float Rigidbody2DNode::GetElasticity() const { return mElasticity; }
	float& Rigidbody2DNode::Elasticity() { return mElasticity; }
	void Rigidbody2DNode::SetElasticity(float elasticity) { mElasticity = elasticity; }

	float Rigidbody2DNode::GetFriction() const { return mFriction; }
	float& Rigidbody2DNode::Friction() { return mFriction; }
	void Rigidbody2DNode::SetFriction(float friction) { mFriction = friction; }

	Vector2f Rigidbody2DNode::GetLinearVelocity() const { return mLinearVelocity; }
	Vector2f& Rigidbody2DNode::GetLinearVelocity() { return mLinearVelocity; }
	void Rigidbody2DNode::SetLinearVelocity(Vector2f linearVelocity) { mLinearVelocity = linearVelocity; }

	float Rigidbody2DNode::GetAngularVelocity() const { return mAngularVelocity; }
	float& Rigidbody2DNode::AngularVelocity() { return mAngularVelocity; }
	void Rigidbody2DNode::SetAngularVelocity(float angularVelocity) { mAngularVelocity = angularVelocity; }
}
