#include "puffin/nodes/node.h"

#include "puffin/scene/scenegraphsubsystem.h"
#include "puffin/core/engine.h"
#include "puffin/ecs/enttsubsystem.h"

namespace puffin
{
	Node::Node(const std::shared_ptr<core::Engine>& engine, const UUID& id) :
		mNodeID(id), mEntity(), mEngine(engine)
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		mRegistry = enttSubsystem->GetRegistry();
	}

	void Node::Initialize()
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		mEntity = enttSubsystem->AddEntity(mNodeID);
	}

	void Node::Deinitialize()
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		enttSubsystem->RemoveEntity(mNodeID);
	}

	void Node::BeginPlay()
	{
	}

	void Node::EndPlay()
	{
	}

	void Node::Update(double deltaTime)
	{
	}

	bool Node::ShouldUpdate() const
	{
		return false;
	}

	void Node::FixedUpdate(double deltaTime)
	{
	}

	bool Node::ShouldFixedUpdate() const
	{
		return false;
	}

	void Node::Serialize(serialization::Archive& archive) const
	{
		archive.Serialize("name", mName);
	}

	void Node::Deserialize(const serialization::Archive& archive)
	{
		archive.Deserialize("name", mName);
	}

	UUID Node::GetID() const
	{
		return mNodeID;
	}

	entt::entity Node::GetEntity() const
	{
		return mEntity;
	}

	const std::string& Node::GetName() const
	{
		return mName;
	}

	void Node::SetName(const std::string& name)
	{
		mName = name;
	}

	void Node::QueueDestroy() const
	{
		auto sceneGraph = mEngine->GetSubsystem<scene::SceneGraphSubsystem>();
		sceneGraph->QueueDestroyNode(mNodeID);
	}

	Node* Node::GetParent() const
	{
		if (mParentID != gInvalidID)
		{
			auto sceneGraph = mEngine->GetSubsystem<scene::SceneGraphSubsystem>();
			return sceneGraph->GetNode(mParentID);
		}

		return nullptr;
	}

	void Node::Reparent(const UUID& id)
	{
		if (mParentID != gInvalidID)
		{
			auto parent = GetParent();
			if (parent)
				parent->RemoveChildID(mNodeID);
		}

		SetParentID(id);
	}

	void Node::GetChildren(std::vector<Node*>& children) const
	{
		children.reserve(mChildIDs.size());

		auto sceneGraph = mEngine->GetSubsystem<scene::SceneGraphSubsystem>();

		for (auto id : mChildIDs)
		{
			children.push_back(sceneGraph->GetNode(id));
		}
	}

	void Node::GetChildIDs(std::vector<UUID>& childIDs) const
	{
		childIDs.reserve(mChildIDs.size());

		for (auto id : mChildIDs)
		{
			childIDs.push_back(id);
		}
	}

	Node* Node::GetChild(UUID id) const
	{
		auto sceneGraph = mEngine->GetSubsystem<scene::SceneGraphSubsystem>();
		return sceneGraph->GetNode(id);
	}

	void Node::RemoveChild(UUID id)
	{
		auto sceneGraph = mEngine->GetSubsystem<scene::SceneGraphSubsystem>();
		Node* node = sceneGraph->GetNode(id);
		node->QueueDestroy();

		RemoveChildID(id);
	}

	UUID Node::GetParentID() const
	{
		return mParentID;
	}

	void Node::SetParentID(UUID id)
	{
		mParentID = id;
	}

	void Node::AddChildID(UUID id)
	{
		mChildIDs.push_back(id);
	}

	void Node::RemoveChildID(UUID id)
	{
		mChildIDs.remove(id);
	}
}
