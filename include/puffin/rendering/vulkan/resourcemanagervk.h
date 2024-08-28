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

		void AddImage(const std::string& name, const ImageDescVK & imageDesc);
		void AddBuffer(const std::string& name, const BufferDescVK& bufferDesc);

		ResourceID AddImages(const ImageDesc& imageDesc, uint8_t imageCount);

		void DestroyImages(ResourceID id);

		void UpdateImage(ResourceID id, const ImageDesc& imageDesc, uint8_t imageIdx);
		void UpdateImages(ResourceID id, const ImageDesc& imageDesc);

		AllocatedImage& GetImage(ResourceID id, uint8_t idx = 0);
		[[nodiscard]] bool IsImageValid(ResourceID id) const;
		[[nodiscard]] bool IsImageValid(ResourceID id, uint8_t idx) const;
		[[nodiscard]] size_t GetImageCount(ResourceID id) const;

		UnifiedGeometryBuffer* GeometryBuffer() const;

	private:

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
			std::unordered_set<uint8_t> existsInFrames; // Set of frames in which a copy of this resource exists
			ResourceType type;
		};

		struct Resources
		{
			std::unordered_map<std::string, ResourceID> resourceNameToID;
			std::unordered_map<ResourceID, AllocatedImage> images;
			std::unordered_map<ResourceID, AllocatedBuffer> buffers;
		};

		RenderSubsystemVK* mRenderSystem = nullptr;
		UnifiedGeometryBuffer* mUnifiedGeometryBuffer = nullptr;

		std::unordered_map<std::string, ImageDescVK> mImageDescriptions;

		uint8_t mBufferedFrameCount = 0;
		std::unordered_map<std::string, ResourceInfo> mResourceInfo;
		Resources mPersistentResources; // Resources which persist between frames
		std::vector<Resources> mFrameResources; // Resources which change each frame

		MappedVector<ResourceID, std::vector<AllocatedImage>> mImages;

		

	};
}
