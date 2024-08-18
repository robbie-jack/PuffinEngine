#pragma once

#include <unordered_set>
#include <memory>

#include "puffin/types/uuid.h"
#include "puffin/rendering/vulkan/typesvk.h"
#include "puffin/types/packedvector.h"

namespace puffin::rendering
{
	class RenderSubsystemVK;

	class MaterialRegistryVK
	{
	public:

		explicit MaterialRegistryVK(RenderSubsystemVK* renderSystem);
		~MaterialRegistryVK();

		void RegisterMaterialInstance(const UUID& id);

		void Update();

		[[nodiscard]] bool GetMaterialDataNeedsUploaded() const;

		MaterialDataVK& GetMaterialData(const UUID& id);
		PackedVector<UUID, MaterialDataVK>& GetAllMaterialData();

		MaterialVK& GetMaterial(const UUID& id);

		GPUMaterialInstanceData& GetCachedMaterialData(const UUID& id);

	private:

		std::shared_ptr<RenderSubsystemVK> mRenderSystem = nullptr;

		std::unordered_set<UUID> mMaterialsToLoad; // Materials that need to be loaded
		std::unordered_set<UUID> mMaterialsInstancesToLoad; // Materials Instances that need to be loaded

		PackedVector<UUID, MaterialDataVK> mMatData;
		std::unordered_map<UUID, MaterialVK> mMats;

		PackedVector<UUID, GPUMaterialInstanceData> mCachedMaterialData; // Cached data for each unique material/instance

		bool mMaterialDataNeedsUploaded = false;

		bool LoadMaterialInstance(UUID matID, MaterialDataVK& matData);
		void InitMaterialPipeline(UUID matID);
		
	};
}
