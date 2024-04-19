#pragma once

#include "nlohmann/json.hpp"
#include "entt/entity/registry.hpp"

#include "Core/System.h"
#include "ECS/EnTTSubsystem.h"
#include "Components/SceneObjectComponent.h"
#include "Types/UUID.h"

#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace puffin::io
{
	using SceneComponentType = uint16_t;

	// Interface for array to store scene data
	class IComponentDataArray
	{
	public:

		virtual ~IComponentDataArray() = default;

		virtual void init(const std::shared_ptr<ecs::EnTTSubsystem>& enttSubsystem) = 0;
		virtual void update(const std::shared_ptr<ecs::EnTTSubsystem>& enttSubsystem) = 0;
		virtual void clear() = 0;
		virtual size_t size() = 0;
		virtual json saveToJson() const = 0;
		virtual void loadFromJson(const json& componentData) = 0;
	};

	template<typename CompT>
	class ComponentDataArray : public IComponentDataArray
	{
	public:

		ComponentDataArray() {}
		~ComponentDataArray() override = default;

		void init(const std::shared_ptr<ecs::EnTTSubsystem>& enttSubsystem) override
		{
			const auto registry = enttSubsystem->registry();

			for (const auto& [id, comp] : mComponents)
			{
				const auto entity = enttSubsystem->get_entity(id);

				if (registry->any_of<CompT>(entity))
				{
					registry->get<CompT>(entity) = comp;
				}
				else
				{
					registry->emplace_or_replace<CompT>(entity, comp);
				}
			}
		}

		void update(const std::shared_ptr<ecs::EnTTSubsystem>& enttSubsystem) override
		{
			clear();

			const auto registry = enttSubsystem->registry();

			const auto view = registry->view<const SceneObjectComponent>();

			for (auto [entity, object] : view.each())
			{
				if (registry->any_of<CompT>(entity))
				{
					mComponents.emplace(object.id, registry->get<CompT>(entity));
				}
			}
		}

		void clear() override
		{
			mComponents.clear();
		}

		size_t size() override
		{
			return mComponents.size();
		}

		[[nodiscard]] json saveToJson() const override
		{
			json data;

			int i = 0;
			for (auto& [id, comp] : mComponents)
			{
				data[i] = { id, comp };

				i++;
			}

			return data;
		}

		void loadFromJson(const json& componentData) override
		{
			for (const auto& data : componentData)
			{
				PuffinID id = data.at(0);

				mComponents[id] = data.at(1);
			}
		}

	private:

		std::map<PuffinID, CompT> mComponents; // Map of entity id to component

	};

	// Stores Loaded Scene Data
	class SceneData
	{
	public:

		SceneData() = default;
		SceneData(const fs::path& path) : mPath(path) {};

		// Initialize ECS with loaded data
		void init(const std::shared_ptr<ecs::EnTTSubsystem>& enttSubsystem)
		{
			// Add entities to registry/subsystem
			for (const auto& id : mIDs)
			{
				enttSubsystem->add_entity(id);
			}

			// Init components
			for (auto& [type, compArray] : mComponentData)
			{
				compArray->init(enttSubsystem);
			}
		}

		void updateData(const std::shared_ptr<ecs::EnTTSubsystem>& enttSubsystem)
		{
			clear();

			const auto registry = enttSubsystem->registry();

			const auto view = registry->view<const SceneObjectComponent>();

			mIDs.reserve(view.size());

			for (auto [entity, object] : view.each())
			{
				mIDs.push_back(object.id);
			}

			for (auto& [type, compArray] : mComponentData)
			{
				compArray->update(enttSubsystem);
			}

			mHasData = true;
		}

		void clear()
		{
			mIDs.clear();

			for (auto& [type, compArray] : mComponentData)
			{
				compArray->clear();
			}

			mHasData = false;
		}

		template<typename CompT>
		void registerComponent()
		{
			const char* typeName = typeid(CompT).name();

			assert(mComponentData.find(typeName) == mComponentData.end() && "Registering component type more than once");

			// Create
			std::shared_ptr<ComponentDataArray<CompT>> sceneDataArray = std::make_shared<ComponentDataArray<CompT>>();

			mComponentData.insert({ typeName, std::static_pointer_cast<IComponentDataArray>(sceneDataArray) });
		}

		// Save Entities/Components to json scene file
		void save()
		{
			// Write scene data to json file
			
			json data;
			data["IDs"] = mIDs;

			for (auto& [fst, snd] : mComponentData)
			{
				if (snd->size() > 0)
					data[fst] = snd->saveToJson();
			}

			if (!fs::exists(mPath.parent_path()))
			{
				fs::create_directories(mPath.parent_path());
			}

			std::ofstream os(mPath, std::ios::out);

			os << std::setw(4) << data << std::endl;

			os.close();
		}

		// Load  Entities/Components from Binary Scene File
		void load(const bool forceLoad = false)
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

			mIDs = data.at("IDs").get<std::vector<PuffinID>>();

			for (auto& [type, compArray] : mComponentData)
			{
				if (data.contains(type))
				{
					compArray->loadFromJson(data.at(type));
				}
			}

			mHasData = true;
		}

		void loadAndInit(const std::shared_ptr<ecs::EnTTSubsystem>& enttSubsystem)
		{
			load();
			init(enttSubsystem);
		}

		void setPath(const fs::path& path)
		{
			mPath = path;
		}

		const fs::path& path()
		{
			return mPath;
		}

	private:

		fs::path mPath;
		bool mHasData = false; // This scene contains a copy of active scene data, either loaded from file or copied from ecs

		std::vector<PuffinID> mIDs;
		std::unordered_map<std::string, std::shared_ptr<IComponentDataArray>> mComponentData; // Map of component data arrays

	};

	class SceneSubsystem : public core::System
	{
	public:

		SceneSubsystem(const std::shared_ptr<core::Engine>& engine) : System(engine)
		{
			mEngine->registerCallback(core::ExecutionStage::Startup, [&] { loadAndInit(); }, "SceneSubsystem: LoadAndInit", 200);
			mEngine->registerCallback(core::ExecutionStage::BeginPlay, [&] { beginPlay(); }, "SceneSubsystem: BeginPlay", 0);
			mEngine->registerCallback(core::ExecutionStage::EndPlay, [&] { loadAndInit(); }, "SceneSubsystem: LoadAndInit", 200);
		}

		~SceneSubsystem() override { mEngine = nullptr; }

		void loadAndInit() const
		{
			const auto subsystem = mEngine->getSystem<ecs::EnTTSubsystem>();

			mSceneData->load();
			mSceneData->init(subsystem);
		}

		void beginPlay() const
		{
			const auto subsystem = mEngine->getSystem<ecs::EnTTSubsystem>();

			mSceneData->updateData(subsystem);
		}

		std::shared_ptr<SceneData> createScene(const fs::path& path)
		{
			mSceneData = std::make_shared<SceneData>(path);

			return mSceneData;
		}

		std::shared_ptr<SceneData> sceneData() { return mSceneData; }

	private:

		std::shared_ptr<SceneData> mSceneData = nullptr;

	};
}