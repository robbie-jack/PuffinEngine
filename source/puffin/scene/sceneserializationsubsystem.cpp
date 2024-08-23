#include "puffin/scene/sceneserializationsubsystem.h"

namespace puffin::io
{
	SceneData::SceneData(SceneSerializationSubsystem* sceneSerializationSubsystem, fs::path path) :
		mSceneSerializationSubsystem(sceneSerializationSubsystem),
		mPath(std::move(path))
	{
		
	}

	void SceneData::Setup(ecs::EnTTSubsystem* enttSubsystem, scene::SceneGraphSubsystem* sceneGraph)
	{
		auto registry = enttSubsystem->GetRegistry();

		// Add entities & components to registry/subsystem
		for (const auto& id : mEntityIDs)
		{
			auto entity = enttSubsystem->AddEntity(id);

			for (const auto& [typeID, serialComp] : mSceneSerializationSubsystem->GetSerializableComponents())
			{
				const auto& entityArchiveMap = mEntityArchiveMaps.at(typeID);
				if (entityArchiveMap.find(id) != entityArchiveMap.end())	
				{
					const auto& archive = entityArchiveMap.at(id);

					serialComp->Deserialize(registry, entity, archive);
				}
			}
		}

		// Add nodes to scene graph
		for (const auto& id : mRootNodeIDs)
		{
			const auto& serializedNodeData = mSerializedNodeData.at(id);

			auto node = sceneGraph->AddNode(serializedNodeData.type.c_str(), id);
			node->Deserialize(serializedNodeData.archive);

			for (const auto& childID : serializedNodeData.childIDs)
			{
				const auto& serializedNodeDataChild = mSerializedNodeData.at(childID);

				auto childNode = sceneGraph->AddChildNode(serializedNodeDataChild.type.c_str(), childID, id);
				childNode->Deserialize(serializedNodeDataChild.archive);
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

			for (const auto& [typeID, serialComp] : mSceneSerializationSubsystem->GetSerializableComponents())
			{
				if (mEntityArchiveMaps.find(typeID) == mEntityArchiveMaps.end())
				{
					mEntityArchiveMaps.emplace(typeID, EntityArchiveMap());
				}

				if (serialComp->HasComponent(registry, entity))
				{
					auto& entityArchiveMap = mEntityArchiveMaps.at(typeID);
					if (entityArchiveMap.find(id) == entityArchiveMap.end())
					{
						entityArchiveMap.emplace(id, serialization::Archive());
					}

					entityArchiveMap.at(id).Clear();
					serialComp->Serialize(registry, entity, entityArchiveMap.at(id));
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
		mEntityArchiveMaps.clear();
		mRootNodeIDs.clear();
		mNodeIDs.clear();
		mSerializedNodeData.clear();

		mHasData = false;
	}

	void SceneData::Save()
	{
		// Write scene data to json file

		// Write entities & components
		json data;
		data["entityIDs"] = mEntityIDs;
		
		for (const auto& [typeID, entityArchiveMap] : mEntityArchiveMaps)
		{
			if (!entityArchiveMap.empty())
			{
				auto type = entt::resolve(typeID);

				json componentJson;

				int i = 0;
				for (const auto& [id, archive] : entityArchiveMap)
				{
					json archiveJson;
					archive.DumpToJson(archiveJson);

					componentJson[i] = { id, archiveJson };

					++i;
				}

				data[type.info().name()] = componentJson;
			}
		}

		// Write nodes
		data["rootNodeIDs"] = mRootNodeIDs;

		std::vector<json> nodeJsons;
		nodeJsons.resize(mNodeIDs.size());

		int i = 0;
		for (const auto& id : mNodeIDs)
		{
			const auto& serializedNodeData = mSerializedNodeData.at(id);
			auto& nodeJson = nodeJsons[i];

			nodeJson["id"] = id;
			nodeJson["type"] = serializedNodeData.type;

			json nodeDataJson;
			serializedNodeData.archive.DumpToJson(nodeDataJson);
			nodeJson["data"] = nodeDataJson;

			if (!serializedNodeData.childIDs.empty())
			{
				nodeJson["childIDs"] = serializedNodeData.childIDs;
			}

			++i;
		}

		data["nodes"] = nodeJsons;

		if (!fs::exists(mPath.parent_path()))
		{
			fs::create_directories(mPath.parent_path());
		}

		std::ofstream os(mPath, std::ios::out);

		os << std::setw(4) << data << std::endl;

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

		json data;
		is >> data;

		is.close();

		mEntityIDs = data.at("entityIDs").get<std::vector<UUID>>();

		for (const auto& [typeID, serialComp] : mSceneSerializationSubsystem->GetSerializableComponents())
		{
			if (mEntityArchiveMaps.find(typeID) == mEntityArchiveMaps.end())
			{
				mEntityArchiveMaps.emplace(typeID, EntityArchiveMap());
			}

			auto type = entt::resolve(typeID);
			const auto& typeName = type.info().name();

			if (data.contains(typeName))
			{
				const json& componentJson = data.at(typeName);
				for (const auto& archiveJson : componentJson)
				{
					UUID id = archiveJson.at(0);

					auto& entityArchiveMap = mEntityArchiveMaps.at(typeID);
					if (entityArchiveMap.find(id) == entityArchiveMap.end())
					{
						entityArchiveMap.emplace(id, serialization::Archive());
					}

					entityArchiveMap.at(id).PopulateFromJson(archiveJson.at(1));
				}
			}
		}

		mRootNodeIDs = data.at("rootNodeIDs").get<std::vector<UUID>>();

		for (const auto& nodeJson : data.at("nodes"))
		{
			const UUID& id = nodeJson.at("id");

			mNodeIDs.push_back(id);
			mSerializedNodeData.emplace(id, SerializedNodeData{});

			auto& serializedNodeData = mSerializedNodeData.at(id);
			serializedNodeData.id = id;
			serializedNodeData.type = nodeJson.at("type");

			const json& nodeDataJson = nodeJson.at("data");
			serializedNodeData.archive.PopulateFromJson(nodeDataJson);

			if (nodeJson.contains("childIDs"))
			{
				serializedNodeData.childIDs = nodeJson.at("childIDs").get<std::vector<UUID>>();
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
		serializedNodeData.type = sceneGraph->GetNodeTypeName(id);
		node->Serialize(serializedNodeData.archive);

		for (auto childID : serializedNodeData.childIDs)
		{
			SerializeNodeAndChildren(sceneGraph, childID);
		}
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
		mSerializableComponents.clear();
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

	std::shared_ptr<SceneData> SceneSerializationSubsystem::CreateScene(const fs::path& path)
	{
		auto scenePath = (assets::AssetRegistry::Get()->GetContentRoot() / path).make_preferred();

		if (mSceneData.find(scenePath) == mSceneData.end())
		{
			mSceneData.emplace(scenePath, std::make_shared<SceneData>(this, scenePath));
		}

		mCurrentSceneData = mSceneData.at(scenePath);

		return mCurrentSceneData;
	}

	std::shared_ptr<SceneData> SceneSerializationSubsystem::GetSceneData()
	{
		return mCurrentSceneData;
	}

	const SerializableComponentMap& SceneSerializationSubsystem::GetSerializableComponents()
	{
		return mSerializableComponents;
	}
}
