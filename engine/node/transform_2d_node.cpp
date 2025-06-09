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

		AddComponent<TransformComponent2D>();
	}

	void Transform2DNode::Deinitialize()
	{
		Node::Deinitialize();

		RemoveComponent<TransformComponent2D>();
	}

	void Transform2DNode::Update(double deltaTime)
	{
		Node::Update(deltaTime);

		UpdateLocalTransform();
		UpdateGlobalTransform();
	}

	const std::string& Transform2DNode::GetTypeString() const
	{
		return gTransformNode2DTypeString;
	}

	entt::id_type Transform2DNode::GetTypeID() const
	{
		return reflection::GetTypeHashedString<Transform2DNode>();
	}

	const Transform2D& Transform2DNode::GetTransform() const
	{
		return mLocalTransform;
	}

	Transform2D& Transform2DNode::Transform()
	{
		mShouldUpdateGlobalTransform = true;
		return mLocalTransform;
	}

	void Transform2DNode::SetTransform(const Transform2D& transform)
	{
		mShouldUpdateGlobalTransform = true;
		mLocalTransform = transform;
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
	

	const Vector2f& Transform2DNode::GetPosition() const
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

	const float& Transform2DNode::GetRotation() const
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

	const Vector2f& Transform2DNode::GetScale() const
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

	const Transform2D& Transform2DNode::GetGlobalTransform() const
	{
		return mGlobalTransform;
	}

	Transform2D& Transform2DNode::GlobalTransform()
	{
		
		return mGlobalTransform;
	}

	void Transform2DNode::SetGlobalTransform(const Transform2D& transform)
	{
		mShouldUpdateLocalTransform = true;
		mGlobalTransform = transform;
	}

	void Transform2DNode::UpdateLocalTransform()
	{
		if (!mShouldUpdateLocalTransform)
			return;

		// PUFFIN_TODO - Implement

		mShouldUpdateLocalTransform = false;
	}

	void Transform2DNode::UpdateGlobalTransform()
	{
		if (!mShouldUpdateGlobalTransform)
			return;

		// PUFFIN_TODO - Implement

		mShouldUpdateGlobalTransform = false;
	}
}
