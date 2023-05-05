#pragma once

#include "ECS/ECS.h"
#include "nlohmann/json.hpp"

#include <set>
#include <map>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace puffin::io
{
	// Interface for array to store scene data
	class ISceneDataArray
	{
	public:

		virtual ~ISceneDataArray() = default;
		virtual void InitSceneData() = 0;
		virtual void UpdateData() = 0;
		virtual void Clear() = 0;
		virtual int Size() = 0;
		virtual json SaveToJson() const = 0;
		virtual void LoadFromJson(const std::string& componentType, const json& data) = 0;
	};

	template<typename CompT>
	class SceneDataArray : public ISceneDataArray
	{
	public:

		SceneDataArray(std::shared_ptr<ECS::World> inWorld) : m_world(inWorld) {}
		~SceneDataArray() override = default;

		void InitSceneData() override
		{
			for (auto& pair : m_componentMap)
			{
				CompT& comp = m_world->AddAndGetComponent<CompT>(pair.first);
				comp = pair.second;
			}
		}

		void UpdateData() override
		{
			Clear();

			const std::set<ECS::EntityID> entities = m_world->GetActiveEntities();

			for (auto entity : entities)
			{
				if (m_world->HasComponent<CompT>(entity))
				{
					auto& comp = m_world->GetComponent<CompT>(entity);
					m_componentMap.insert({ entity, comp });
				}
			}
		}

		void Clear() override
		{
			m_componentMap.clear();
		}

		int Size() override
		{
			return m_componentMap.size();
		}

		json SaveToJson() const override
		{
			json data;

			int i = 0;
			for (auto& [fst, snd] : m_componentMap)
			{
				data[i] = { fst, snd };

				i++;
			}

			return data;
		}

		void LoadFromJson(const std::string& componentType, const json& data) override
		{
			if (!data.contains(componentType))
				return;

			const int size = data[componentType].size();

			for (int i = 0; i < size; i++)
			{
				ECS::EntityID entity = data[componentType].at(i).at(0);

				m_componentMap[entity] = data[componentType].at(i).at(1);
			}
		}

	private:

		std::shared_ptr<ECS::World> m_world; // Pointer to ECS World
		std::map<ECS::EntityID, CompT> m_componentMap; // Map of entity id to component

	};

	struct EntityData
	{
		ECS::EntityID id;
		std::string name;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(EntityData, id, name)
	};

	// Stores Loaded Scene Data
	class SceneData
	{
	public:

		SceneData(std::shared_ptr<ECS::World> inWorld, fs::path inPath) : m_world(inWorld), m_scenePath(inPath) {}

		// Initialize ECS with loaded data
		void Init()
		{
			// Initialize ECS World with Entities
			std::set<ECS::EntityID> entities;
			for (auto& entityData : m_entityData)
			{
				entities.insert(entityData.id);
			}

			m_world->InitEntitySystem(entities);

			// Set Entity Names
			for (auto& entityData : m_entityData)
			{
				m_world->SetEntityName(entityData.id, entityData.name);
			}

			// Init Each Component Type
			for (auto& pair : m_sceneDataArrays)
			{
				pair.second->InitSceneData();
			}
		}

		void UpdateData()
		{
			Clear();

			std::set<ECS::EntityID> entities = m_world->GetActiveEntities();

			for (ECS::EntityID entity : entities)
			{
				EntityData data;
				data.id = entity;
				data.name = m_world->GetEntityName(entity);

				m_entityData.push_back(data);
			}

			for (auto& [fst, snd] : m_sceneDataArrays)
			{
				snd->UpdateData();
			}
		}

		void Clear()
		{
			m_entityData.clear();

			for (auto& pair : m_sceneDataArrays)
			{
				pair.second->Clear();
			}
		}

		template<typename CompT>
		void RegisterComponent()
		{
			const char* typeName = typeid(CompT).name();

			assert(m_sceneDataArrays.find(typeName) == m_sceneDataArrays.end() && "Registering component type more than once");

			// Create
			std::shared_ptr<SceneDataArray<CompT>> sceneDataArray = std::make_shared<SceneDataArray<CompT>>(m_world);

			m_sceneDataArrays.insert({ typeName, std::static_pointer_cast<ISceneDataArray>(sceneDataArray) });
		}

		// Save Entities/Components to Binary Scene File
		void Save()
		{
			UpdateData();

			// Initialize Output File Stream and Cereal Binary Archive
			
			json data;
			data["Entities"] = m_entityData;

			for (auto& [fst, snd] : m_sceneDataArrays)
			{
				if (snd->Size() > 0)
					data[fst] = snd->SaveToJson();
			}

			std::ofstream os(m_scenePath, std::ios::out);

			os << std::setw(4) << data << std::endl;

			os.close();
		}

		// Load  Entities/Components from Binary Scene File
		void Load()
		{
			if (!fs::exists(m_scenePath))
				return;

			// Initialize Input File Stream and Cereal Binary Archive
			std::ifstream is(m_scenePath);

			json data;
			is >> data;

			is.close();

			m_entityData = data["Entities"];

			for (auto& [fst, snd] : m_sceneDataArrays)
			{
				snd->LoadFromJson(fst, data);
			}
		}

		void LoadAndInit()
		{
			Load();
			Init();
		}

		void SetPath(const fs::path& path)
		{
			m_scenePath = path;
		}

		const fs::path& GetPath()
		{
			return m_scenePath;
		}

	private:

		std::shared_ptr<ECS::World> m_world; // Pointer to ECS World
		fs::path m_scenePath;

		std::vector<EntityData> m_entityData;
		std::unordered_map<std::string, std::shared_ptr<ISceneDataArray>> m_sceneDataArrays; // Map of scene data arrays

	};
}