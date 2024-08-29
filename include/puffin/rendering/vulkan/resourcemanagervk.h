#pragma once

#include <memory>
#include <unordered_set>

#include "puffin/rendering/vulkan/typesvk.h"
#include "puffin/types/storage/mappedvector.h"
#include "puffin/types/uuid.h"
#include "puffin/rendering/resourceid.h"

namespace puffin
{
	namespace assets
	{
		class StaticMeshAsset;
	}
}

namespace puffin::rendering
{
	struct BufferDescVK;
	struct ImageDescVK;
	class RenderSubsystemVK;
	class UnifiedGeometryBuffer;

	class ResourceManagerVK
	{
	public:

		explicit ResourceManagerVK(RenderSubsystemVK* renderSystem, uint8_t bufferedFrameCount);
		~ResourceManagerVK();

		void AddStaticMesh(const std::shared_ptr<assets::StaticMeshAsset>& staticMesh);

		[[nodiscard]] ResourceID AddImage(const ImageDescVK& imageDesc);
		[[nodiscard]] ResourceID AddImage(const ImageDescVK& imageDesc, ResourceID id);
		[[nodiscard]] ResourceID AddImage(const ImageDescVK& imageDesc, const std::string& name);

		[[nodiscard]] ResourceID AddBuffer(const BufferDescVK& bufferDesc, const std::string& name);

		void Update();

		ResourceID AddImages(const ImageDesc& imageDesc, uint8_t imageCount);

		void DestroyImages(ResourceID id);

		void UpdateImage(ResourceID id, const ImageDesc& imageDesc, uint8_t imageIdx);
		void UpdateImages(ResourceID id, const ImageDesc& imageDesc);

		AllocatedImage& GetImage(ResourceID id, uint8_t idx = 0);
		[[nodiscard]] bool IsImageValid(ResourceID id) const;
		[[nodiscard]] bool IsImageValid(ResourceID id, uint8_t idx) const;
		[[nodiscard]] size_t GetImageCount(ResourceID id) const;

		[[nodiscard]] bool IsResourceValid(const std::string& name) const;
		[[nodiscard]] bool IsResourceValid(ResourceID id) const;

		UnifiedGeometryBuffer* GeometryBuffer() const;

	private:

		void CreateResourcesInstances();
		void DestroyResourcesInstances();

		void AddImageInternal(const ImageDescVK& imageDesc, ResourceID id, const std::string& name);

		void UpdateImageInternal(const ImageDescVK& imageDesc, ResourceID id);

		void CreateImageInstanceInternal(ResourceID instanceID, ImageDescVK& imageDesc);
		void DestroyImageInstanceInternal(ResourceID instanceID);

		void CreateImageInternal(ResourceID id, const ImageDesc& imageDesc, uint8_t idx = 0);
		void DestroyImageInternal(ResourceID id, uint8_t idx = 0);
		void UpdateImageInternal(ResourceID id, const ImageDesc& imageDesc, uint8_t idx = 0);

		enum class ResourceType
		{
			Image = 0,
			Buffer
		};

		struct ResourceInfo
		{
			std::string name = "";
			ResourceID id = gInvalidID;
			bool persistent = false;
			std::vector<uint8_t> instanceIDs; // IDs of individual instances of this resource
			ResourceType type;
		};

		RenderSubsystemVK* mRenderSystem = nullptr;
		UnifiedGeometryBuffer* mUnifiedGeometryBuffer = nullptr;

		uint8_t mBufferedFrameCount = 0;
		std::unordered_map<ResourceID, ResourceInfo> mResourceInfo;
		std::unordered_map<std::string, ResourceID> mResourceNameToID;

		std::unordered_map<ResourceID, AllocatedImage> mAllocImageInstances;
		std::unordered_map<ResourceID, ImageDescVK> mImageDescInstances;

		std::unordered_map<ResourceID, AllocatedBuffer> mBuffers;
		std::unordered_map<ResourceID, BufferDescVK> mBufferDescs;

		std::unordered_map<ResourceID, ImageDescVK> mImageInstancesToCreate;
		std::vector<std::unordered_set<ResourceID>> mImageInstancesToDestroy;

		MappedVector<ResourceID, std::vector<AllocatedImage>> mImages;
	};
}
