#pragma once

#include <ECS/ECS.h>
#include "nlohmann/json.hpp"

#include <set>
#include <map>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace Puffin::IO
{
	// Interface for array to store scene data
	class ISceneDataArray
	{
	public:

		virtual ~ISceneDataArray() = default;
		virtual void InitSceneData() = 0;
		virtual void UpdateData() = 0;
		virtual void Clear() = 0;
		virtual json SaveToJson() const = 0;
		virtual void LoadFromJson(const std::string& componentType, std::set<ECS::EntityID> entities, const json& data) = 0;
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
				m_world->AddComponent<CompT>(pair.first, pair.second);
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

		void LoadFromJson(const std::string& componentType, std::set<ECS::EntityID> entities, const json& data) override
		{
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

	// Stores Loaded Scene Data
	class SceneData
	{
	public:

		SceneData(std::shared_ptr<ECS::World> inWorld, fs::path inPath) : m_world(inWorld), m_scenePath(inPath) {}

		// Initialize ECS with loaded data
		void Init()
		{
			// Initialize ECS World with Entities
			m_world->InitEntitySystem(m_entities);

			// Set Entity Names
			for (ECS::EntityID entity : m_entities)
			{
				m_world->SetEntityName(entity, m_entityNames[entity]);
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

			m_entities = m_world->GetActiveEntities();

			for (ECS::EntityID entity : m_entities)
			{
				m_entityNames.insert({ entity, m_world->GetEntityName(entity) });
			}

			for (auto& [fst, snd] : m_sceneDataArrays)
			{
				snd->UpdateData();
			}
		}

		void Clear()
		{
			m_entities.clear();
			m_entityNames.clear();

			for (auto& pair : m_sceneDataArrays)
			{
				pair.second->Clear();
			}
		}

		template<typename CompT>
		void RegisterComponent(std::string componentType)
		{
			assert(m_sceneDataArrays.find(componentType) == m_sceneDataArrays.end() && "Registering component type more than once");

			// Create
			std::shared_ptr<SceneDataArray<CompT>> sceneDataArray = std::make_shared<SceneDataArray<CompT>>(m_world);

			m_sceneDataArrays.insert({ componentType, std::static_pointer_cast<ISceneDataArray>(sceneDataArray) });
		}

		// Save Entities/Components to Binary Scene File
		void Save()
		{
			UpdateData();

			// Initialize Output File Stream and Cereal Binary Archive
			
			json data;
			data["Entities"] = m_entities;
			data["EntityNames"] = m_entityNames;

			for (auto& [fst, snd] : m_sceneDataArrays)
			{
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

			const std::set<ECS::EntityID> entities = data["Entities"];
			m_entities = entities;

			const std::unordered_map<ECS::EntityID, std::string> entityNames = data["EntityNames"];
			m_entityNames = entityNames;

			for (auto& [fst, snd] : m_sceneDataArrays)
			{
				snd->LoadFromJson(fst, m_entities, data);
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

		std::set<ECS::EntityID> m_entities;
		std::unordered_map<ECS::EntityID, std::string> m_entityNames;
		std::unordered_map<std::string, std::shared_ptr<ISceneDataArray>> m_sceneDataArrays; // Map of scene data arrays

	};
}