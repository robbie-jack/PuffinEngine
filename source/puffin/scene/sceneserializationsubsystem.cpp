#include "puffin/scene/sceneserializationsubsystem.h"

namespace puffin::io
{
	void SceneData::Setup(ecs::EnTTSubsystem* enttSubsystem, scene::SceneGraphSubsystem* sceneGraph)
	{
		// Add entities to registry/subsystem
		for (const auto& id : mEntityIDs)
		{
			enttSubsystem->AddEntity(id);
		}

		// Init components
		for (auto& [type, compArray] : mComponentData)
		{
			compArray->Init(enttSubsystem);
		}

		// Add nodes to scene graph
		for (const auto& id : mRootNodeIDs)
		{
			sceneGraph->AddNode(mNodeIDToType.at(id).c_str(), id);

			if (mChildNodeIDs.count(id) != 0)
			{
				for (const auto& child_id : mChildNodeIDs.at(id))
				{
					sceneGraph->AddChildNode(mNodeIDToType.at(child_id).c_str(), child_id, id);
				}
			}
		}

		for (const auto& id : mNodeIDs)
		{
			auto node = sceneGraph->GetNode(id);
			node->Deserialize(mNodeIDToJson.at(id));
		}
	}

	void SceneData::UpdateData(ecs::EnTTSubsystem* enttSubsystem, scene::SceneGraphSubsystem* sceneGraph)
	{
		Clear();

		const auto registry = enttSubsystem->GetRegistry();

		registry->each([&](auto entity)
		{
			const auto& id = enttSubsystem->GetID(entity);

			mEntityIDs.push_back(id);
		});

		for (auto& [type, compArray] : mComponentData)
		{
			compArray->Update(enttSubsystem);
		}

		for (auto id : sceneGraph->GetRootNodeIDs())
		{
			mRootNodeIDs.push_back(id);

			AddNodeIDAndChildIDs(sceneGraph, id);
		}

		mHasData = true;
	}

	void SceneData::Clear()
	{
		mEntityIDs.clear();

		for (auto& [type, compArray] : mComponentData)
		{
			compArray->Clear();
		}

		mNodeIDs.clear();
		mNodeIDToType.clear();
		mNodeIDToJson.clear();

		mRootNodeIDs.clear();
		mChildNodeIDs.clear();

		mHasData = false;
	}

	void SceneData::Save()
	{
		// Write scene data to json file
			
		json data;
		data["entityIDs"] = mEntityIDs;

		for (auto& [type, comp_array] : mComponentData)
		{
			if (comp_array->GetSize() > 0)
				data[type] = comp_array->SaveToJson();
		}

		data["rootNodeIDs"] = mRootNodeIDs;
		data["nodeIDs"] = mNodeIDs;
		data["nodeIDToType"] = mNodeIDToType;
		data["NodeIDToJson"] = mNodeIDToJson;
		data["childNodeIDs"] = mChildNodeIDs;

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

		for (auto& [type, comp_array] : mComponentData)
		{
			if (data.contains(type))
			{
				comp_array->LoadFromJson(data.at(type));
			}
		}

		mRootNodeIDs = data.at("rootNodeIDs").get<std::vector<UUID>>();
		mNodeIDs = data.at("nodeIDs").get<std::vector<UUID>>();
		mNodeIDToType = data.at("nodeIDToType").get<std::unordered_map<UUID, std::string>>();
		mNodeIDToJson = data.at("NodeIDToJson").get<std::unordered_map<UUID, json>>();
		mChildNodeIDs = data.at("childNodeIDs").get<std::unordered_map<UUID, std::vector<UUID>>>();

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

	void SceneData::AddNodeIDAndChildIDs(scene::SceneGraphSubsystem* sceneGraph, UUID id)
	{
		auto node = sceneGraph->GetNode(id);

		json json;
		node->Serialize(json);

		std::vector<UUID> childIDs;
		node->GetChildIDs(childIDs);

		mNodeIDs.push_back(id);
		mNodeIDToType.insert({ id, sceneGraph->GetNodeTypeName(id) });
		mNodeIDToJson.insert({ id, json });

		if (!childIDs.empty())
			mChildNodeIDs.insert({ id, childIDs });

		for (auto childID : childIDs)
		{
			AddNodeIDAndChildIDs(sceneGraph, childID);
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
	}

	void SceneSerializationSubsystem::BeginPlay()
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto sceneGraph = mEngine->GetSubsystem<scene::SceneGraphSubsystem>();

		mCurrentSceneData->UpdateData(enttSubsystem, sceneGraph);
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

			for (auto& [type_name, scene_comp_register] : mComponentRegisters)
			{
				scene_comp_register->RegisterComponentWithScene(mSceneData.at(scenePath));
			}
		}

		mCurrentSceneData = mSceneData.at(scenePath);

		return mCurrentSceneData;
	}

	std::shared_ptr<SceneData> SceneSerializationSubsystem::GetSceneData()
	{
		return mCurrentSceneData;
	}
}
