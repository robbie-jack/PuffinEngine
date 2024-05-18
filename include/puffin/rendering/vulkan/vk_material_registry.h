#pragma once

#include <unordered_set>

#include "puffin/types/uuid.h"
#include "puffin/rendering/vulkan/vk_types.h"
#include "puffin/types/packed_vector.h"

namespace puffin::rendering
{
	class RenderSystemVK;

	class VKMaterialRegistry
	{
	public:

		VKMaterialRegistry() = default;
		~VKMaterialRegistry() = default;

		void init(const std::shared_ptr<RenderSystemVK>& renderSystem);

		void registerMaterialInstance(const PuffinID& id);

		void update();

		bool materialDataNeedsUploaded() const { return mMaterialDataNeedsUploaded; }

		MaterialDataVK& getMaterialData(const PuffinID& id) { return mMatData[id]; }
		PackedVector<PuffinID, MaterialDataVK>& materialData() { return mMatData; }

		MaterialVK& getMaterial(const PuffinID& id) { return mMats[id]; }

		GPUMaterialInstanceData& getCachedMaterialData(const PuffinID& id) { return mCachedMaterialData[id]; }

	private:

		std::shared_ptr<RenderSystemVK> mRenderSystem = nullptr;

		std::unordered_set<PuffinID> mMaterialsToLoad; // Materials that need to be loaded
		std::unordered_set<PuffinID> mMaterialsInstancesToLoad; // Materials Instances that need to be loaded

		PackedVector<PuffinID, MaterialDataVK> mMatData;
		std::unordered_map<PuffinID, MaterialVK> mMats;

		PackedVector<PuffinID, GPUMaterialInstanceData> mCachedMaterialData; // Cached data for each unique material/instance

		bool mMaterialDataNeedsUploaded = false;

		bool loadMaterialInstance(PuffinID matID, MaterialDataVK& matData);
		void initMaterialPipeline(PuffinID matID);
		
	};
}
