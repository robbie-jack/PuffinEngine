#include "scene/scene_serialization_subsystem.h"

#include "resource/resource_manager.h"
#include "serialization/component_serialization.h"

namespace puffin::scene
{
	SceneData::SceneData(fs::path path)
		: m_path(std::move(path))
	{
	}

	SceneData::SceneData(fs::path path, SceneInfo sceneInfo)
		: m_path(std::move(path)), m_sceneInfo(sceneInfo)
	{
	}

	void SceneData::Setup(ecs::EnTTSubsystem* enttSubsystem, scene::SceneGraphSubsystem* sceneGraph)
	{
		auto registry = enttSubsystem->GetRegistry();

		// Add entities & components to registry/subsystem
		for (const auto& id : m_entityIDs)
		{
			auto entity = enttSubsystem->AddEntity(id);

			auto* componentRegistry = serialization::ComponentRegistry::Get();
			for (const auto& typeID : componentRegistry->GetRegisteredTypesVector())
			{
				const auto& entityJsonMap = m_serializedComponentData.at(typeID);
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
		for (const auto& id : m_rootNodeIDs)
		{
			const auto& serializedNodeData = m_serializedNodeData.at(id);

			auto type = entt::resolve(entt::hs(serializedNodeData.type.c_str()));
			auto typeID = type.id();

			auto node = sceneGraph->AddNode(typeID, serializedNodeData.name, serializedNodeData.id);
			node->Deserialize(serializedNodeData.json);

			for (const auto& childID : serializedNodeData.childIDs)
			{
				const auto& serializedNodeDataChild = m_serializedNodeData.at(childID);

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

			if (!enttSubsystem->ShouldEntityBeSerialized(id))
				continue;

			m_entityIDs.push_back(id);

			auto* componentRegistry = serialization::ComponentRegistry::Get();
			for (const auto& typeID : componentRegistry->GetRegisteredTypesVector())
			{
				if (m_serializedComponentData.find(typeID) == m_serializedComponentData.end())
				{
					m_serializedComponentData.emplace(typeID, EntityJsonMap());
				}

				auto type = entt::resolve(typeID);

				auto hasComponentFunc = type.func(entt::hs("HasComponent"));
				if (hasComponentFunc && hasComponentFunc.invoke({}, registry, entity).cast<bool>())
				{
					if (auto serializefunc = type.func(entt::hs("SerializeFromRegistry")))
					{
						auto& entityJsonMap = m_serializedComponentData.at(typeID);

						entityJsonMap.emplace(id, serializefunc.invoke({}, registry, entity).cast<nlohmann::json>());
					}
				}
			}
		}

		for (auto id : sceneGraph->GetRootNodeIDs())
		{
			m_rootNodeIDs.push_back(id);

			SerializeNodeAndChildren(sceneGraph, id);
		}

		m_hasData = true;
	}

	void SceneData::Clear()
	{
		m_entityIDs.clear();
		m_serializedComponentData.clear();
		m_rootNodeIDs.clear();
		m_nodeIDs.clear();
		m_serializedNodeData.clear();

		m_hasData = false;
	}

	void SceneData::Save()
	{
		// Serialize scene data to json file
		nlohmann::json sceneJson;

		// Serialize scene info
		sceneJson["sceneInfo"] = SerializeSceneInfo(m_sceneInfo);

		// Serialize entities & components
		nlohmann::json componentsJson;
		componentsJson["entityIDs"] = m_entityIDs;
		
		for (const auto& [typeID, entityJsonMap] : m_serializedComponentData)
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

		nodesJson["rootNodeIDs"] = m_rootNodeIDs;

		std::vector<nlohmann::json> nodeJsons;
		nodeJsons.resize(m_nodeIDs.size());

		int i = 0;
		for (const auto& id : m_nodeIDs)
		{
			const auto& serializedNodeData = m_serializedNodeData.at(id);
			
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
		if (!fs::exists(m_path.parent_path()))
		{
			fs::create_directories(m_path.parent_path());
		}

		std::ofstream os(m_path, std::ios::out);

		os << std::setw(4) << sceneJson << std::endl;

		os.close();
	}

	void SceneData::Load(const bool forceLoad)
	{
		if (m_hasData && !forceLoad)
			return;

		if (!fs::exists(m_path))
			return;

		// Initialize Input File Stream and Cereal Binary Archive
		std::ifstream is(m_path);

		json sceneJson;
		is >> sceneJson;

		is.close();

		m_sceneInfo = DeserializeSceneInfo(sceneJson.at("sceneInfo"));

		// Deserialize Components

		nlohmann::json componentsJson = sceneJson.at("components");
		m_entityIDs = componentsJson.at("entityIDs").get<std::vector<UUID>>();

		auto* componentRegistry = serialization::ComponentRegistry::Get();
		for (const auto& typeID : componentRegistry->GetRegisteredTypesVector())
		{
			if (m_serializedComponentData.find(typeID) == m_serializedComponentData.end())
			{
				m_serializedComponentData.emplace(typeID, EntityJsonMap());
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
						auto& entityJsonMap = m_serializedComponentData.at(typeID);
						entityJsonMap.emplace(archiveJson["id"], archiveJson["data"]);
					}
				}
			}
		}

		// Deserialize Nodes
		nlohmann::json nodesJson = sceneJson.at("nodes");

		m_rootNodeIDs = nodesJson.at("rootNodeIDs").get<std::vector<UUID>>();

		for (const auto& nodeJson : nodesJson.at("nodes"))
		{
			const UUID& id = nodeJson.at("id");

			m_nodeIDs.push_back(id);
			m_serializedNodeData.emplace(id, SerializedNodeData{});

			auto& serializedNodeData = m_serializedNodeData.at(id);
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

		m_hasData = true;
	}

	void SceneData::LoadAndInit(ecs::EnTTSubsystem* enttSubsystem, scene::SceneGraphSubsystem* sceneGraph)
	{
		Load();
		Setup(enttSubsystem, sceneGraph);
	}

	void SceneData::SetPath(const fs::path& path)
	{
		m_path = path;
	}

	const fs::path& SceneData::GetPath()
	{
		return m_path;
	}

	const SceneInfo& SceneData::GetSceneInfo() const
	{
		return m_sceneInfo;
	}

	void SceneData::SerializeNodeAndChildren(scene::SceneGraphSubsystem* sceneGraph, UUID id)
	{
		auto node = sceneGraph->GetNode(id);

		m_nodeIDs.push_back(id);
		m_serializedNodeData.emplace(id, SerializedNodeData{});

		auto& serializedNodeData = m_serializedNodeData.at(id);
		serializedNodeData.id = id;
		serializedNodeData.name = node->GetName();
		serializedNodeData.type = node->GetTypeString();

		node->Serialize(serializedNodeData.json);

		for (const auto& childID : node->GetChildIDs())
		{
			serializedNodeData.childIDs.push_back(childID);
			SerializeNodeAndChildren(sceneGraph, childID);
		}
	}

	SceneSerializationSubsystem::SceneSerializationSubsystem(const std::shared_ptr<core::Engine>& engine) : EngineSubsystem(engine)
	{
	}

	SceneSerializationSubsystem::~SceneSerializationSubsystem()
	{
		m_engine = nullptr;
	}

	void SceneSerializationSubsystem::PreInitialize(core::SubsystemManager* subsystemManager)
	{
		EngineSubsystem::PreInitialize(subsystemManager);

		subsystemManager->CreateAndPreInitializeSubsystem<ecs::EnTTSubsystem>();
		subsystemManager->CreateAndPreInitializeSubsystem<scene::SceneGraphSubsystem>();
	}

	void SceneSerializationSubsystem::Initialize()
	{
		EngineSubsystem::Initialize();
	}

	void SceneSerializationSubsystem::Deinitialize()
	{
		m_currentSceneData = nullptr;
		m_sceneData.clear();
	}

	void SceneSerializationSubsystem::BeginPlay()
	{
		const auto enttSubsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto sceneGraph = m_engine->GetSubsystem<scene::SceneGraphSubsystem>();

		m_currentSceneData->UpdateData(m_engine);
	}

	void SceneSerializationSubsystem::EndPlay()
	{
		LoadAndSetup();
	}

	void SceneSerializationSubsystem::Load() const
	{
		m_currentSceneData->Load();
	}

	void SceneSerializationSubsystem::LoadFromFile(const fs::path& path)
	{
		std::shared_ptr<SceneData> scene = nullptr;

		if (m_sceneData.find(path) == m_sceneData.end())
		{
			m_sceneData.emplace(path, std::make_shared<SceneData>(path));
		}

		scene = m_sceneData.at(path);
		scene->Load();

		m_currentSceneData = scene;
	}

	void SceneSerializationSubsystem::Setup() const
	{
		const auto enttSubsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto sceneGraph = m_engine->GetSubsystem<scene::SceneGraphSubsystem>();

		m_currentSceneData->Setup(enttSubsystem, sceneGraph);
	}

	void SceneSerializationSubsystem::LoadAndSetup() const
	{
		Load();
		Setup();
	}

	std::shared_ptr<SceneData> SceneSerializationSubsystem::CreateScene(const fs::path& path, const SceneInfo& sceneInfo)
	{
		auto* resourceManager = m_engine->GetResourceManager();

		auto scenePath = (resourceManager->GetProjectPath() / path).make_preferred();

		if (m_sceneData.find(scenePath) == m_sceneData.end())
		{
			m_sceneData.emplace(scenePath, std::make_shared<SceneData>(scenePath, sceneInfo));
		}

		m_currentSceneData = m_sceneData.at(scenePath);

		return m_currentSceneData;
	}

	std::shared_ptr<SceneData> SceneSerializationSubsystem::GetCurrentSceneData()
	{
		return m_currentSceneData;
	}
}
