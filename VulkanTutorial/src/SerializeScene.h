#pragma once

#ifndef SERIALIZE_SCENE_H
#define SERIALIZE_SCENE_H

#include <ECS/ECS.h>
#include <Components/TransformComponent.h>
#include <Components/Rendering/MeshComponent.h>
#include <Components/Rendering/LightComponent.h>
#include <Components/Physics/RigidbodyComponent2D.h>
#include <Components/Physics/ShapeComponent2D.h>
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
			std::map<ECS::Entity, Physics::RigidbodyComponent2D> rigidbody2DMap;
			std::map<ECS::Entity, Physics::ShapeComponent2D> shape2DMap;
			std::map<ECS::Entity, Scripting::AngelScriptComponent> scriptMap;
		};

		inline static void ClearSceneData(SceneData& sceneData)
		{
			sceneData.entities.clear();
			sceneData.entity_names.clear();
			sceneData.transformMap.clear();
			sceneData.meshMap.clear();
			sceneData.lightMap.clear();
			sceneData.rigidbody2DMap.clear();
			sceneData.shape2DMap.clear();
			sceneData.scriptMap.clear();
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

				if (world->HasComponent<Physics::RigidbodyComponent2D>(entity))
				{
					Physics::RigidbodyComponent2D& comp = world->GetComponent<Physics::RigidbodyComponent2D>(entity);
					sceneData.rigidbody2DMap.insert({ entity, comp });
				}

				if (world->HasComponent<Physics::ShapeComponent2D>(entity))
				{
					Physics::ShapeComponent2D& comp = world->GetComponent<Physics::ShapeComponent2D>(entity);
					sceneData.shape2DMap.insert({entity, comp});
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
			archive(sceneData.rigidbody2DMap);
			archive(sceneData.shape2DMap);
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
			archive(sceneData.rigidbody2DMap);
			archive(sceneData.shape2DMap);
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
				if (sceneData.rigidbody2DMap.find(entity) != sceneData.rigidbody2DMap.end())
				{
					world->AddComponent<Physics::RigidbodyComponent2D>(entity, sceneData.rigidbody2DMap.at(entity));
				}

				// Shape Component 2D
				if (sceneData.shape2DMap.find(entity) != sceneData.shape2DMap.end())
				{
					world->AddComponent<Physics::ShapeComponent2D>(entity, sceneData.shape2DMap.at(entity));
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