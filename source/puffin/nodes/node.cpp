#include "puffin/nodes/node.h"

#include "puffin/scene/scenegraphsubsystem.h"
#include "puffin/core/engine.h"
#include "puffin/ecs/enttsubsystem.h"

namespace puffin
{
	void Node::Prepare(const std::shared_ptr<core::Engine>& engine, const std::string& name, UUID id)
	{
		mEngine = engine;

		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		mRegistry = enttSubsystem->GetRegistry();

		mNodeID = id;

		if (name.empty())
		{
			mName = "Node";
		}
		else
		{
			mName = name;
		}
	}

	void Node::Reset()
	{
		mRegistry = nullptr;
		mEngine = nullptr;
		mNodeID = gInvalidID;
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
		mEntity = {};
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

	void Node::Serialize(nlohmann::json& json) const
	{
	}

	void Node::Deserialize(const nlohmann::json& json)
	{
	}

	const std::string& Node::GetTypeString() const
	{
		return gNodeTypeString;
	}

	entt::id_type Node::GetTypeID() const
	{
		return gNodeTypeID;
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

	const std::list<UUID>& Node::GetChildIDs() const
	{
		return mChildIDs;
	}

	bool Node::HasChildren() const
	{
		return !mChildIDs.empty();
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
