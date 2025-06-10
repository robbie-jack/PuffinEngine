#include "node/transform_2d_node.h"

#include "core/engine.h"
#include "component/transform_component_2d.h"
#include "scene/scene_graph_subsystem.h"

namespace puffin::core
{
	class Engine;
}

namespace puffin
{
	void Transform2DNode::Initialize()
	{
		Node::Initialize();

		mShouldUpdateGlobalTransform = true;
	}

	void Transform2DNode::Deinitialize()
	{
		Node::Deinitialize();

		mLocalTransform = {};
		mGlobalTransform = {};

		mShouldUpdateLocalTransform = false;
		mShouldUpdateGlobalTransform = false;
	}

	void Transform2DNode::Update(double deltaTime)
	{
		Node::Update(deltaTime);

		UpdateLocalTransform();
		UpdateGlobalTransform();
	}

	bool Transform2DNode::ShouldUpdate() const
	{
		return true;
	}

	void Transform2DNode::Serialize(nlohmann::json& json) const
	{
		Node::Serialize(json);

		json["transform"] = serialization::Serialize(mLocalTransform);
	}

	void Transform2DNode::Deserialize(const nlohmann::json& json)
	{
		Node::Deserialize(json);

		mLocalTransform = serialization::Deserialize<Transform2D>(json["transform"]);

		UpdateGlobalTransform(true);
	}

	const std::string& Transform2DNode::GetTypeString() const
	{
		return gTransformNode2DTypeString;
	}

	entt::id_type Transform2DNode::GetTypeID() const
	{
		return reflection::GetTypeHashedString<Transform2DNode>();
	}

	const Transform2D& Transform2DNode::GetTransform()
	{
		UpdateLocalTransform();

		return mLocalTransform;
	}

	Transform2D& Transform2DNode::Transform()
	{
		UpdateLocalTransform();

		mShouldUpdateGlobalTransform = true;

		return mLocalTransform;
	}

	void Transform2DNode::SetTransform(const Transform2D& transform)
	{
		mLocalTransform = transform;

		mShouldUpdateGlobalTransform = true;
	}

#ifdef PFN_DOUBLE_PRECISION
	const Vector2d& Transform2DNode::GetPosition() const
	{
		return GetTransform().position;
	}

	Vector2d& Transform2DNode::Position()
	{
		return Transform().position;
	}

	void Transform2DNode::SetPosition(const Vector2d& position)
	{
		Transform().position = position;
	}
#else
	const Vector2f& Transform2DNode::GetPosition()
	{
		return GetTransform().position;
	}

	Vector2f& Transform2DNode::Position()
	{
		return Transform().position;
	}

	void Transform2DNode::SetPosition(const Vector2f& position)
	{
		Transform().position = position;
	}
#endif

	const float& Transform2DNode::GetRotation()
	{
		return GetTransform().rotation;
	}

	float& Transform2DNode::Rotation()
	{
		return Transform().rotation;
	}

	void Transform2DNode::SetRotation(const float& rotation)
	{
		Transform().rotation = rotation;
	}

	const Vector2f& Transform2DNode::GetScale()
	{
		return GetTransform().scale;
	}

	Vector2f& Transform2DNode::Scale()
	{
		return Transform().scale;
	}

	void Transform2DNode::SetScale(const Vector2f& scale)
	{
		Transform().scale = scale;
	}

	const Transform2D& Transform2DNode::GetGlobalTransform()
	{
		UpdateGlobalTransform();

		return mGlobalTransform;
	}

	Transform2D& Transform2DNode::GlobalTransform()
	{
		UpdateGlobalTransform();
		
		return mGlobalTransform;
	}

	void Transform2DNode::SetGlobalTransform(const Transform2D& transform)
	{
		mShouldUpdateLocalTransform = true;

		mGlobalTransform = transform;
	}

#ifdef PFN_DOUBLE_PRECISION
	const Vector2d& Transform2DNode::GetGlobalPosition()
	{
		return GetGlobalTransform().position;
	}

	Vector2d& Transform2DNode::GlobalPosition()
	{
		return GlobalTransform().position;
	}

	void Transform2DNode::SetGlobalPosition(const Vector2d& position)
	{
		GlobalTransform().position = position;
	}
#else
	const Vector2f& Transform2DNode::GetGlobalPosition()
	{
		return GetGlobalTransform().position;
	}

	Vector2f& Transform2DNode::GlobalPosition()
	{
		return GlobalTransform().position;
	}

	void Transform2DNode::SetGlobalPosition(const Vector2f& position)
	{
		GlobalTransform().position = position;
	}
#endif

	const float& Transform2DNode::GetGlobalRotation()
	{
		return GetGlobalTransform().rotation;
	}

	float& Transform2DNode::GlobalRotation()
	{
		return GlobalTransform().rotation;
	}

	void Transform2DNode::SetGlobalRotation(const float& rotation)
	{
		GlobalTransform().rotation = rotation;
	}

	const Vector2f& Transform2DNode::GetGlobalScale()
	{
		return GetGlobalTransform().scale;
	}

	Vector2f& Transform2DNode::GlobalScale()
	{
		return GlobalTransform().scale;
	}

	void Transform2DNode::SetGlobalScale(const Vector2f& scale)
	{
		GlobalTransform().scale = scale;
	}

	void Transform2DNode::NotifyLocalTransformChanged()
	{
		mShouldUpdateLocalTransform = true;
	}

	void Transform2DNode::NotifyGlobalTransformChanged()
	{
		mShouldUpdateGlobalTransform = true;
	}

	void Transform2DNode::UpdateLocalTransform(bool forceUpdate)
	{
		if (!mShouldUpdateLocalTransform && !forceUpdate)
			return;

		if (auto* parent = dynamic_cast<Transform2DNode*>(GetParent()); parent)
		{
			mLocalTransform = ApplyGlobalToLocalTransform(mGlobalTransform, parent->GetGlobalTransform());
		}
		else
		{
			mGlobalTransform = ApplyGlobalToLocalTransform(mGlobalTransform, {});
		}

		mShouldUpdateLocalTransform = false;
	}

	void Transform2DNode::UpdateGlobalTransform(bool forceUpdate)
	{
		if (!mShouldUpdateGlobalTransform && !forceUpdate)
			return;

		if (auto* parent = dynamic_cast<Transform2DNode*>(GetParent()); parent)
		{
			mGlobalTransform = ApplyLocalToGlobalTransform(mLocalTransform, parent->GetGlobalTransform());
		}
		else
		{
			mGlobalTransform = ApplyLocalToGlobalTransform(mLocalTransform, {});
		}
	
		mShouldUpdateGlobalTransform = false;
	}
}
