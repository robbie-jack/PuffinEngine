#include "puffin/scene/sceneserializationsubsystem.h"

#include "puffin/serialization/componentserialization.h"

namespace puffin::scene
{
	SceneData::SceneData(fs::path path)
		: mPath(std::move(path))
	{
	}

	SceneData::SceneData(fs::path path, SceneInfo sceneInfo)
		: mPath(std::move(path)), mSceneInfo(sceneInfo)
	{
	}

	void SceneData::Setup(ecs::EnTTSubsystem* enttSubsystem, scene::SceneGraphSubsystem* sceneGraph)
	{
		auto registry = enttSubsystem->GetRegistry();

		// Add entities & components to registry/subsystem
		for (const auto& id : mEntityIDs)
		{
			auto entity = enttSubsystem->AddEntity(id);

			auto* componentRegistry = serialization::ComponentRegistry::Get();
			for (const auto& typeID : componentRegistry->GetRegisteredTypesVector())
			{
				const auto& entityJsonMap = mSerializedComponentData.at(typeID);
				if (entityJsonMap.find(id) != entityJsonMap.end())
				{
					auto type = entt::resolve(typeID);
					const auto& json = entityJsonMap.at(id);

					if (auto deserializeToRegistryFunc = type.func(entt::hs("DeserializeToRegistry")))
					{
						deserializeToRegistryFunc.invoke({}, registry, entity, json);
					}
				}
			}
		}

		// Add nodes to scene graph
		for (const auto& id : mRootNodeIDs)
		{
			const auto& serializedNodeData = mSerializedNodeData.at(id);

			auto type = entt::resolve(entt::hs(serializedNodeData.type.c_str()));
			auto typeID = type.id();

			auto node = sceneGraph->AddNode(typeID, serializedNodeData.name, serializedNodeData.id);
			node->Deserialize(serializedNodeData.json);

			for (const auto& childID : serializedNodeData.childIDs)
			{
				const auto& serializedNodeDataChild = mSerializedNodeData.at(childID);

				auto childType = entt::resolve(entt::hs(serializedNodeDataChild.type.c_str()));
				auto childTypeID = childType.id();

				auto childNode = sceneGraph->AddChildNode(childTypeID, serializedNodeDataChild.name, childID, id);
				childNode->Deserialize(serializedNodeDataChild.json);
			}
		}
	}

	void SceneData::UpdateData(const ::std::shared_ptr<core::Engine>& engine)
	{
		Clear();

		auto enttSubsystem = engine->GetSubsystem<ecs::EnTTSubsystem>();
		auto sceneGraph = engine->GetSubsystem<scene::SceneGraphSubsystem>();

		const auto registry = enttSubsystem->GetRegistry();

		for (const auto entity : registry->view<entt::entity>())
		{
			const auto& id = enttSubsystem->GetID(entity);

			mEntityIDs.push_back(id);

			auto* componentRegistry = serialization::ComponentRegistry::Get();
			for (const auto& typeID : componentRegistry->GetRegisteredTypesVector())
			{
				if (mSerializedComponentData.find(typeID) == mSerializedComponentData.end())
				{
					mSerializedComponentData.emplace(typeID, EntityJsonMap());
				}

				auto type = entt::resolve(typeID);

				auto hasComponentFunc = type.func(entt::hs("HasComponent"));
				if (hasComponentFunc && hasComponentFunc.invoke({}, registry, entity).cast<bool>())
				{
					if (auto serializefunc = type.func(entt::hs("SerializeFromRegistry")))
					{
						auto& entityJsonMap = mSerializedComponentData.at(typeID);

						entityJsonMap.emplace(id, serializefunc.invoke({}, registry, entity).cast<nlohmann::json>());
					}
				}
			}
		}

		for (auto id : sceneGraph->GetRootNodeIDs())
		{
			mRootNodeIDs.push_back(id);

			SerializeNodeAndChildren(sceneGraph, id);
		}

		mHasData = true;
	}

	void SceneData::Clear()
	{
		mEntityIDs.clear();
		mSerializedComponentData.clear();
		mRootNodeIDs.clear();
		mNodeIDs.clear();
		mSerializedNodeData.clear();

		mHasData = false;
	}

	void SceneData::Save()
	{
		// Serialize scene data to json file
		nlohmann::json sceneJson;

		// Serialize scene info
		sceneJson["sceneInfo"] = SerializeSceneInfo(mSceneInfo);

		// Serialize entities & components
		nlohmann::json componentsJson;
		componentsJson["entityIDs"] = mEntityIDs;
		
		for (const auto& [typeID, entityJsonMap] : mSerializedComponentData)
		{
			if (!entityJsonMap.empty())
			{
				auto type = entt::resolve(typeID);

				nlohmann::json componentJson;

				int i = 0;
				for (const auto& [id, json] : entityJsonMap)
				{
					componentJson[i]["id"] = id;
					componentJson[i]["data"] = json;

					++i;
				}

				if (auto getTypeStringFunc = type.func(entt::hs("GetTypeString")))
				{
					auto typeString = getTypeStringFunc.invoke({}).cast<std::string_view>();

					componentsJson[typeString] = componentJson;
				}
			}
		}

		sceneJson["components"] = componentsJson;

		// Serialize nodes
		nlohmann::json nodesJson;

		nodesJson["rootNodeIDs"] = mRootNodeIDs;

		std::vector<nlohmann::json> nodeJsons;
		nodeJsons.resize(mNodeIDs.size());

		int i = 0;
		for (const auto& id : mNodeIDs)
		{
			const auto& serializedNodeData = mSerializedNodeData.at(id);
			
			auto& nodeJson = nodeJsons[i];

			nodeJson["id"] = serializedNodeData.id;
			nodeJson["name"] = serializedNodeData.name;
			nodeJson["type"] = serializedNodeData.type;
			
			if (!serializedNodeData.json.empty())
			{
				nodeJson["data"] = serializedNodeData.json;
			}

			if (!serializedNodeData.childIDs.empty())
			{
				nodeJson["childIDs"] = serializedNodeData.childIDs;
			}

			++i;
		}

		nodesJson["nodes"] = nodeJsons;
		sceneJson["nodes"] = nodesJson;

		// Write scene data to file
		if (!fs::exists(mPath.parent_path()))
		{
			fs::create_directories(mPath.parent_path());
		}

		std::ofstream os(mPath, std::ios::out);

		os << std::setw(4) << sceneJson << std::endl;

		os.close();
	}

	void SceneData::Load(const bool forceLoad)
	{
		if (mHasData && !forceLoad)
			return;

		if (!fs::exists(mPath))
			return;

		// Initialize Input File Stream and Cereal Binary Archive
		std::ifstream is(mPath);

		json sceneJson;
		is >> sceneJson;

		is.close();

		// Deserialize Components

		nlohmann::json componentsJson = sceneJson.at("components");
		mEntityIDs = componentsJson.at("entityIDs").get<std::vector<UUID>>();

		auto* componentRegistry = serialization::ComponentRegistry::Get();
		for (const auto& typeID : componentRegistry->GetRegisteredTypesVector())
		{
			if (mSerializedComponentData.find(typeID) == mSerializedComponentData.end())
			{
				mSerializedComponentData.emplace(typeID, EntityJsonMap());
			}

			auto type = entt::resolve(typeID);

			if (auto getTypeStringFunc = type.func(entt::hs("GetTypeString")))
			{
				auto typeString = getTypeStringFunc.invoke({}).cast<std::string_view>();

				if (componentsJson.contains(typeString))
				{
					const json& componentJson = componentsJson.at(typeString);
					for (const auto& archiveJson : componentJson)
					{
						auto& entityJsonMap = mSerializedComponentData.at(typeID);
						entityJsonMap.emplace(archiveJson["id"], archiveJson["data"]);
					}
				}
			}
		}

		// Deserialize Nodes
		nlohmann::json nodesJson = sceneJson.at("nodes");

		mRootNodeIDs = nodesJson.at("rootNodeIDs").get<std::vector<UUID>>();

		for (const auto& nodeJson : nodesJson.at("nodes"))
		{
			const UUID& id = nodeJson.at("id");

			mNodeIDs.push_back(id);
			mSerializedNodeData.emplace(id, SerializedNodeData{});

			auto& serializedNodeData = mSerializedNodeData.at(id);
			serializedNodeData.id = id;
			serializedNodeData.name = nodeJson.at("name");
			serializedNodeData.type = nodeJson.at("type");
			
			if (nodeJson.contains("childIDs"))
			{
				serializedNodeData.childIDs = nodeJson.at("childIDs").get<std::vector<UUID>>();
			}

			if (nodeJson.contains("data"))
			{
				serializedNodeData.json = nodeJson.at("data");
			}
		}

		mHasData = true;
	}

	void SceneData::LoadAndInit(ecs::EnTTSubsystem* enttSubsystem, scene::SceneGraphSubsystem* sceneGraph)
	{
		Load();
		Setup(enttSubsystem, sceneGraph);
	}

	void SceneData::SetPath(const fs::path& path)
	{
		mPath = path;
	}

	const fs::path& SceneData::GetPath()
	{
		return mPath;
	}

	void SceneData::SerializeNodeAndChildren(scene::SceneGraphSubsystem* sceneGraph, UUID id)
	{
		auto node = sceneGraph->GetNode(id);

		mNodeIDs.push_back(id);
		mSerializedNodeData.emplace(id, SerializedNodeData{});

		auto& serializedNodeData = mSerializedNodeData.at(id);
		serializedNodeData.id = id;
		serializedNodeData.name = node->GetName();
		serializedNodeData.type = node->GetTypeString();
		
		for (const auto& childID : node->GetChildIDs())
		{
			serializedNodeData.childIDs.push_back(childID);
			SerializeNodeAndChildren(sceneGraph, childID);
		}

		node->Serialize(serializedNodeData.json);
	}

	SceneSerializationSubsystem::SceneSerializationSubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
		mName = "SceneSubsystem";
	}

	SceneSerializationSubsystem::~SceneSerializationSubsystem()
	{
		mEngine = nullptr;
	}

	void SceneSerializationSubsystem::Initialize(core::SubsystemManager* subsystemManager)
	{
		subsystemManager->CreateAndInitializeSubsystem<ecs::EnTTSubsystem>();
		subsystemManager->CreateAndInitializeSubsystem<scene::SceneGraphSubsystem>();
	}

	void SceneSerializationSubsystem::Deinitialize()
	{
		mCurrentSceneData = nullptr;
		mSceneData.clear();
	}

	void SceneSerializationSubsystem::BeginPlay()
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto sceneGraph = mEngine->GetSubsystem<scene::SceneGraphSubsystem>();

		mCurrentSceneData->UpdateData(mEngine);
	}

	void SceneSerializationSubsystem::EndPlay()
	{
		LoadAndSetup();
	}

	void SceneSerializationSubsystem::Load() const
	{
		mCurrentSceneData->Load();
	}

	void SceneSerializationSubsystem::LoadFromFile(const fs::path& path)
	{
		std::shared_ptr<SceneData> scene = nullptr;

		if (mSceneData.find(path) == mSceneData.end())
		{
			mSceneData.emplace(path, std::make_shared<SceneData>(path));
		}

		scene = mSceneData.at(path);
		scene->Load();

		mCurrentSceneData = scene;
	}

	void SceneSerializationSubsystem::Setup() const
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto sceneGraph = mEngine->GetSubsystem<scene::SceneGraphSubsystem>();

		mCurrentSceneData->Setup(enttSubsystem, sceneGraph);
	}

	void SceneSerializationSubsystem::LoadAndSetup() const
	{
		Load();
		Setup();
	}

	std::shared_ptr<SceneData> SceneSerializationSubsystem::CreateScene(const fs::path& path, const SceneInfo& sceneInfo)
	{
		auto scenePath = (assets::AssetRegistry::Get()->GetContentRoot() / path).make_preferred();

		if (mSceneData.find(scenePath) == mSceneData.end())
		{
			mSceneData.emplace(scenePath, std::make_shared<SceneData>(scenePath, sceneInfo));
		}

		mCurrentSceneData = mSceneData.at(scenePath);

		return mCurrentSceneData;
	}

	std::shared_ptr<SceneData> SceneSerializationSubsystem::GetCurrentSceneData()
	{
		return mCurrentSceneData;
	}
}
