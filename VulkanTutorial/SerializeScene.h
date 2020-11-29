#pragma once

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
		// Save Entities/Components to Binary Scene File
		static void SaveScene(std::string scene_name, ECS::World* world)
		{
			// Initialize Output File Stream and Cereal Binary Archive
			std::ofstream os(scene_name, std::ios::binary);
			cereal::BinaryOutputArchive archive(os);

			std::set<ECS::Entity> entities;
			std::map<ECS::Entity, std::string> entity_names;

			// Store Entities, Names and Signatures in local vectors
			for (ECS::Entity entity : world->GetActiveEntities())
			{
				entities.insert(entity);
				entity_names.insert({ entity, world->GetEntityName(entity) });
			}

			// Save Entities, Names and Signatures to Archive
			archive(entities, entity_names);

			std::map<ECS::Entity, TransformComponent> transformMap;
			std::map<ECS::Entity, Rendering::MeshComponent> meshMap;
			std::map<ECS::Entity, Physics::RigidbodyComponent> rigidbodyMap;

			// Store component's in map, with entity as key value
			for (ECS::Entity entity : world->GetActiveEntities())
			{
				if (world->HasComponent<TransformComponent>(entity))
				{
					TransformComponent& comp = world->GetComponent<TransformComponent>(entity);
					transformMap.insert({ entity, comp });
				}

				if (world->HasComponent<Rendering::MeshComponent>(entity))
				{
					Rendering::MeshComponent& comp = world->GetComponent<Rendering::MeshComponent>(entity);
					meshMap.insert({ entity, comp });
				}

				if (world->HasComponent<Physics::RigidbodyComponent>(entity))
				{
					Physics::RigidbodyComponent& comp = world->GetComponent<Physics::RigidbodyComponent>(entity);
					rigidbodyMap.insert({ entity, comp });
				}
			}

			archive(transformMap);
			archive(meshMap);
			archive(rigidbodyMap);
		}

		// Load  Entities/Components from Binary Scene File
		static void LoadScene(std::string scene_name, ECS::World* world)
		{
			// Initialize Input File Stream and Cereal Binary Archive
			std::ifstream is(scene_name, std::ios::binary);
			cereal::BinaryInputArchive archive(is);

			std::set<ECS::Entity> entities;
			std::map<ECS::Entity, std::string> entity_names;

			// Load Entites from Archive
			archive(entities, entity_names);

			std::map<ECS::Entity, TransformComponent> transformMap;
			std::map<ECS::Entity, Rendering::MeshComponent> meshMap;
			std::map<ECS::Entity, Physics::RigidbodyComponent> rigidbodyMap;

			// Load Components into Maps
			archive(transformMap);
			archive(meshMap);
			archive(rigidbodyMap);

			// Initiliase EntityManager with Existing Entities
			world->InitEntitySystem(entities);

			// Initilize ECS with all loaded data
			for (ECS::Entity entity : entities)
			{
				world->SetEntityName(entity, entity_names.at(entity));

				// If Entity has Transform Component, Insert into ECS
				if (transformMap.find(entity) != transformMap.end())
				{
					world->AddComponent<TransformComponent>(entity, transformMap.at(entity));
				}

				// If Entity has Mesh Component, Insert into ECS
				if (meshMap.find(entity) != meshMap.end())
				{
					world->AddComponent<Rendering::MeshComponent>(entity, meshMap.at(entity));
				}

				// If Entity has Rigidbody Component, Insert into ECS
				if (rigidbodyMap.find(entity) != rigidbodyMap.end())
				{
					world->AddComponent<Physics::RigidbodyComponent>(entity, rigidbodyMap.at(entity));
				}
			}
		}
	}
}