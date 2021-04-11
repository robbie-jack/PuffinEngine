#pragma once

#ifndef SERIALIZE_SCENE_H
#define SERIALIZE_SCENE_H

#include <ECS/ECS.h>
#include <Components/TransformComponent.h>
#include <Components/Rendering/MeshComponent.h>
#include <Components/Rendering/LightComponent.h>
#include <Components/Physics/RigidbodyComponent.h>
#include <Components/AngelScriptComponent.h>

#include <vector>
#include <set>
#include <map>
#include <fstream>

#include <cereal/types/vector.hpp>
#include <cereal/types/set.hpp>
#include <cereal/types/map.hpp>
#include <cereal/archives/binary.hpp>

namespace Puffin
{
	namespace IO
	{
		// Stores Loaded Scene Data
		struct SceneData
		{
			std::string scene_name;
			std::set<ECS::Entity> entities;
			std::map<ECS::Entity, std::string> entity_names;
			std::map<ECS::Entity, TransformComponent> transformMap;
			std::map<ECS::Entity, Rendering::MeshComponent> meshMap;
			std::map<ECS::Entity, Rendering::LightComponent> lightMap;
			std::map<ECS::Entity, Physics::RigidbodyComponent> rigidbodyMap;
			std::map<ECS::Entity, Scripting::AngelScriptComponent> scriptMap;
		};

		inline static void ClearSceneData(SceneData& sceneData)
		{
			sceneData.entities.clear();
			sceneData.entity_names.clear();
			sceneData.transformMap.clear();
			sceneData.meshMap.clear();
			sceneData.lightMap.clear();
			sceneData.rigidbodyMap.clear();
		}

		inline static void UpdateSceneData(std::shared_ptr<ECS::World> world, SceneData& sceneData)
		{
			// Clear Out Old Scene Data
			ClearSceneData(sceneData);

			// Store Entities, Names and Signatures in local vectors
			for (ECS::Entity entity : world->GetActiveEntities())
			{
				sceneData.entities.insert(entity);
				sceneData.entity_names.insert({ entity, world->GetEntityName(entity) });
			}

			// Store component's in map, with entity as key value
			for (ECS::Entity entity : world->GetActiveEntities())
			{
				if (world->HasComponent<TransformComponent>(entity))
				{
					TransformComponent& comp = world->GetComponent<TransformComponent>(entity);
					sceneData.transformMap.insert({ entity, comp });
				}

				if (world->HasComponent<Rendering::MeshComponent>(entity))
				{
					Rendering::MeshComponent& comp = world->GetComponent<Rendering::MeshComponent>(entity);
					sceneData.meshMap.insert({ entity, comp });
				}

				if (world->HasComponent<Rendering::LightComponent>(entity))
				{
					Rendering::LightComponent& comp = world->GetComponent<Rendering::LightComponent>(entity);
					sceneData.lightMap.insert({ entity, comp });
				}

				if (world->HasComponent<Physics::RigidbodyComponent>(entity))
				{
					Physics::RigidbodyComponent& comp = world->GetComponent<Physics::RigidbodyComponent>(entity);
					sceneData.rigidbodyMap.insert({ entity, comp });
				}

				if (world->HasComponent<Scripting::AngelScriptComponent>(entity))
				{
					Scripting::AngelScriptComponent& comp = world->GetComponent<Scripting::AngelScriptComponent>(entity);
					sceneData.scriptMap.insert({ entity, comp });
				}
			}
		}

		// Save Entities/Components to Binary Scene File
		inline static void SaveScene(std::shared_ptr<ECS::World> world, SceneData& sceneData)
		{
			// Initialize Output File Stream and Cereal Binary Archive
			std::ofstream os(sceneData.scene_name, std::ios::binary);
			cereal::BinaryOutputArchive archive(os);

			UpdateSceneData(world, sceneData);

			// Save Entities and Components to Archive
			archive(sceneData.entities, sceneData.entity_names);
			archive(sceneData.transformMap);
			archive(sceneData.meshMap);
			archive(sceneData.lightMap);
			archive(sceneData.rigidbodyMap);
			archive(sceneData.scriptMap);
		}

		// Load  Entities/Components from Binary Scene File
		inline static void LoadScene(std::shared_ptr<ECS::World> world, SceneData& sceneData)
		{
			// Initialize Input File Stream and Cereal Binary Archive
			std::ifstream is(sceneData.scene_name, std::ios::binary);
			cereal::BinaryInputArchive archive(is);

			// Clear Old Scene Data
			ClearSceneData(sceneData);

			// Load Entites from Archive
			archive(sceneData.entities, sceneData.entity_names);

			// Load Components into Maps
			archive(sceneData.transformMap);
			archive(sceneData.meshMap);
			archive(sceneData.lightMap);
			archive(sceneData.rigidbodyMap);
			archive(sceneData.scriptMap);
		}

		// Initialize ECS with loaded data
		inline static void InitScene(std::shared_ptr<ECS::World> world, SceneData& sceneData)
		{
			// Initiliase EntityManager with Existing Entities
			world->InitEntitySystem(sceneData.entities);

			// Initilize ECS with all loaded data
			for (ECS::Entity entity : sceneData.entities)
			{
				world->SetEntityName(entity, sceneData.entity_names.at(entity));

				// If Entity has Transform Component, Insert into ECS
				if (sceneData.transformMap.find(entity) != sceneData.transformMap.end())
				{
					world->AddComponent<TransformComponent>(entity, sceneData.transformMap.at(entity));
				}

				// If Entity has Mesh Component, Insert into ECS
				if (sceneData.meshMap.find(entity) != sceneData.meshMap.end())
				{
					world->AddComponent<Rendering::MeshComponent>(entity, sceneData.meshMap.at(entity));
				}

				// If Entity has Light Component, Insert into ECS
				if (sceneData.lightMap.find(entity) != sceneData.lightMap.end())
				{
					world->AddComponent<Rendering::LightComponent>(entity, sceneData.lightMap.at(entity));
				}

				// If Entity has Rigidbody Component, Insert into ECS
				if (sceneData.rigidbodyMap.find(entity) != sceneData.rigidbodyMap.end())
				{
					world->AddComponent<Physics::RigidbodyComponent>(entity, sceneData.rigidbodyMap.at(entity));
				}

				// Script Component
				if (sceneData.scriptMap.find(entity) != sceneData.scriptMap.end())
				{
					world->AddComponent<Scripting::AngelScriptComponent>(entity, sceneData.scriptMap.at(entity));
				}
			}
		}
	}
}

#endif // !SERIALIZE_SCENE_H