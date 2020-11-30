#pragma once

#ifndef SERIALIZE_SCENE_H
#define SERIALIZE_SCENE_H

#include "ECS.h"
#include "TransformComponent.h"
#include "MeshComponent.h"
#include "RigidbodyComponent.h"

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
			std::map<ECS::Entity, Physics::RigidbodyComponent> rigidbodyMap;
		};

		// Save Entities/Components to Binary Scene File
		static void SaveScene(std::string scene_name, ECS::World* world, SceneData& sceneData)
		{
			sceneData.scene_name = scene_name;

			// Initialize Output File Stream and Cereal Binary Archive
			std::ofstream os(sceneData.scene_name, std::ios::binary);
			cereal::BinaryOutputArchive archive(os);

			// Store Entities, Names and Signatures in local vectors
			for (ECS::Entity entity : world->GetActiveEntities())
			{
				sceneData.entities.insert(entity);
				sceneData.entity_names.insert({ entity, world->GetEntityName(entity) });
			}

			// Save Entities, Names and Signatures to Archive
			archive(sceneData.entities, sceneData.entity_names);

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

				if (world->HasComponent<Physics::RigidbodyComponent>(entity))
				{
					Physics::RigidbodyComponent& comp = world->GetComponent<Physics::RigidbodyComponent>(entity);
					sceneData.rigidbodyMap.insert({ entity, comp });
				}
			}

			archive(sceneData.transformMap);
			archive(sceneData.meshMap);
			archive(sceneData.rigidbodyMap);
		}

		// Load  Entities/Components from Binary Scene File
		static void LoadScene(std::string scene_name, ECS::World* world, SceneData& sceneData)
		{
			sceneData.scene_name = scene_name;

			// Initialize Input File Stream and Cereal Binary Archive
			std::ifstream is(sceneData.scene_name, std::ios::binary);
			cereal::BinaryInputArchive archive(is);

			// Load Entites from Archive
			archive(sceneData.entities, sceneData.entity_names);

			// Load Components into Maps
			archive(sceneData.transformMap);
			archive(sceneData.meshMap);
			archive(sceneData.rigidbodyMap);

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

				// If Entity has Rigidbody Component, Insert into ECS
				if (sceneData.rigidbodyMap.find(entity) != sceneData.rigidbodyMap.end())
				{
					world->AddComponent<Physics::RigidbodyComponent>(entity, sceneData.rigidbodyMap.at(entity));
				}
			}
		}

		// Initialize ECS with loaded data
		static void InitScene(ECS::World* world, SceneData& sceneData)
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

				// If Entity has Rigidbody Component, Insert into ECS
				if (sceneData.rigidbodyMap.find(entity) != sceneData.rigidbodyMap.end())
				{
					world->AddComponent<Physics::RigidbodyComponent>(entity, sceneData.rigidbodyMap.at(entity));
				}
			}
		}
	}
}

#endif // !SERIALIZE_SCENE_H