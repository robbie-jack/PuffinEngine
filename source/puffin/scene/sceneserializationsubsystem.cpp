#include "puffin/scene/sceneserializationsubsystem.h"

#include "puffin/serialization/componentserialization.h"

namespace puffin::io
{
	SceneData::SceneData(fs::path path) :
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

			// PFN_TODO_SERIALIZATION - Re-implement using new serialization logic
			/*for (const auto& [typeID, serialComp] : mSceneSerializationSubsystem->GetSerializableComponents())
			{
				const auto& entityJsonMap = mEntityJsonMaps.at(typeID);
				if (entityJsonMap.find(id) != entityJsonMap.end())	
				{
					const auto& json = entityJsonMap.at(id);

					serialComp->Deserialize(registry, entity, json);
				}
			}*/
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
				auto type = entt::resolve(typeID);

				if (mEntityJsonMaps.find(typeID) == mEntityJsonMaps.end())
				{
					mEntityJsonMaps.emplace(typeID, EntityJsonMap());
				}

				auto hasComponentFunc = type.func(entt::hs("HasComponent"));
				if (hasComponentFunc && hasComponentFunc.invoke({}, registry, entity).cast<bool>())
				{
					if (auto serializefunc = type.func(entt::hs("SerializeFromRegistry")))
					{
						auto& entityJsonMap = mEntityJsonMaps.at(typeID);

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
		mEntityJsonMaps.clear();
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
		
		for (const auto& [typeID, entityJsonMap] : mEntityJsonMaps)
		{
			if (!entityJsonMap.empty())
			{
				auto type = entt::resolve(typeID);

				json componentJson;

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

					data[typeString] = componentJson;
				}
			}
		}

		// Write nodes
		data["rootNodeIDs"] = mRootNodeIDs;

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

		// PFN_TODO_SERIALIZATION - Re-implement using new serialization logic
		/*for (const auto& [typeID, serialComp] : mSceneSerializationSubsystem->GetSerializableComponents())
		{
			if (mEntityJsonMaps.find(typeID) == mEntityJsonMaps.end())
			{
				mEntityJsonMaps.emplace(typeID, EntityJsonMap());
			}

			auto type = entt::resolve(typeID);
			const auto& typeName = type.info().name();

			if (data.contains(typeName))
			{
				const json& componentJson = data.at(typeName);
				for (const auto& archiveJson : componentJson)
				{
					auto& entityJsonMap = mEntityJsonMaps.at(typeID);
					entityJsonMap.emplace(archiveJson["id"], archiveJson["data"]);
				}
			}
		}*/

		mRootNodeIDs = data.at("rootNodeIDs").get<std::vector<UUID>>();

		for (const auto& nodeJson : data.at("nodes"))
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
			mSceneData.emplace(scenePath, std::make_shared<SceneData>(scenePath));
		}

		mCurrentSceneData = mSceneData.at(scenePath);

		return mCurrentSceneData;
	}

	std::shared_ptr<SceneData> SceneSerializationSubsystem::GetSceneData()
	{
		return mCurrentSceneData;
	}
}
