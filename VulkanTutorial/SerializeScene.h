#pragma once

#include "ECS.h"
#include "TransformComponent.h"
#include "MeshComponent.h"
#include "RigidbodyComponent.h"

#include <vector>
#include <map>
#include <fstream>

#include <cereal/types/vector.hpp>
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

			std::vector<ECS::Entity> entities;
			std::vector<std::string> entity_names;
			std::vector<ECS::Signature> entity_signatures;

			// Store Entities, Names and Signatures in local vectors
			for (ECS::Entity entity : world->GetActiveEntities())
			{
				entities.push_back(entity);
				entity_names.push_back(world->GetEntityName(entity));
				entity_signatures.push_back(world->GetEntitySignature(entity));
			}

			// Save Entities, Names and Signatures to Archive
			archive(entities, entity_names, entity_signatures);

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
			//archive(meshMap);
			//archive(rigidbodyMap);
		}

		// Load  Entities/Components from Binary Scene File
		static void LoadScene(std::string scene_name, ECS::World* world)
		{
			// Initialize Input File Stream and Cereal Binary Archive
			std::ifstream is(scene_name, std::ios::binary);
			cereal::BinaryInputArchive archive(is);

			std::vector<ECS::Entity> entities;
			std::vector<std::string> entity_names;
			std::vector<ECS::Signature> entity_signatures;

			// Load Entites from Archive
			archive(entities, entity_names, entity_signatures);

			std::map<ECS::Entity, TransformComponent> transformMap;
			std::map<ECS::Entity, Rendering::MeshComponent> meshMap;

			archive(transformMap);
			//archive(meshMap);
		}
	}
}