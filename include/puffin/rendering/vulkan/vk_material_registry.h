#pragma once

#include "puffin/types/uuid.h"
#include "puffin/rendering/vulkan/vk_types.h"
#include "puffin/types/packed_array.h"

#include <unordered_set>

namespace puffin::rendering
{
	class VKRenderSystem;

	class VKMaterialRegistry
	{
	public:

		VKMaterialRegistry() = default;
		~VKMaterialRegistry() = default;

		void init(const std::shared_ptr<VKRenderSystem>& renderSystem);

		void registerMaterialInstance(const PuffinID& id);

		void update();

		bool materialDataNeedsUploaded() const { return mMaterialDataNeedsUploaded; }

		MaterialDataVK& getMaterialData(const PuffinID& id) { return mMatData[id]; }
		PackedVector<MaterialDataVK>& materialData() { return mMatData; }

		MaterialVK& getMaterial(const PuffinID& id) { return mMats[id]; }

		GPUMaterialInstanceData& getCachedMaterialData(const PuffinID& id) { return mCachedMaterialData[id]; }

	private:

		std::shared_ptr<VKRenderSystem> mRenderSystem = nullptr;

		std::unordered_set<PuffinID> mMaterialsToLoad; // Materials that need to be loaded
		std::unordered_set<PuffinID> mMaterialsInstancesToLoad; // Materials Instances that need to be loaded

		PackedVector<MaterialDataVK> mMatData;
		PackedVector<MaterialVK> mMats;

		PackedVector<GPUMaterialInstanceData> mCachedMaterialData; // Cached data for each unique material/instance

		bool mMaterialDataNeedsUploaded = false;

		bool loadMaterialInstance(PuffinID matID, MaterialDataVK& matData);
		void initMaterialPipeline(PuffinID matID);
		
	};
}
