#pragma once

#include "Asset.h"

#include "Types/UUID.h"
#include "Rendering/MaterialGlobals.h"
#include "nlohmann/json.hpp"

#include <bitset>

using json = nlohmann::json;


namespace puffin::assets
{
    static const std::string gMaterialAssetType = "Material";
    static constexpr uint32_t gMaterialAssetVersion = 1; // Latest version of Material Asset Format

    class MaterialAsset : public Asset
    {
    public:

        MaterialAsset() : Asset(fs::path()) {}

        explicit MaterialAsset(const fs::path& path) : Asset(path) {}

        MaterialAsset(const PuffinID id, const fs::path& path) : Asset(id, path) {}

        ~MaterialAsset() override = default;

        [[nodiscard]] const std::string& type() const override
        {
            return gMaterialAssetType;
        }

        [[nodiscard]] const uint32_t& version() const override
        {
            return gMaterialAssetVersion;
        }

        bool save();

        bool load();

        void unload() override {}

        [[nodiscard]] PuffinID getVertexShaderID() const { return mVertexShaderID; }
        void setVertexShaderID(const PuffinID vertID) { mVertexShaderID = vertID; }

        [[nodiscard]] PuffinID getFragmentShaderID() const { return mFragmentShaderID; }
        void setFragmentShaderID(const PuffinID fragID) { mFragmentShaderID = fragID; }

        std::array<PuffinID, rendering::gNumTexturesPerMat>& getTexIDs() { return mTexIDs; }
        std::array<float, rendering::gNumFloatsPerMat>& getData() { return mData; }

        std::array<bool, rendering::gNumTexturesPerMat>& getTexIDOverride() { return mTexIDOverride; }
        std::array<bool, rendering::gNumFloatsPerMat>& getDataOverride() { return mDataOverride; }

        [[nodiscard]] PuffinID getBaseMaterialID() const { return mBaseMaterial; }
        void setBaseMaterialID(const PuffinID matID) { mBaseMaterial = matID; }

    private:

        PuffinID mVertexShaderID = gInvalidID;
        PuffinID mFragmentShaderID = gInvalidID;

        // Base material this instance overrides. If this is gInvalidID, then it has no base material
        PuffinID mBaseMaterial = gInvalidID; 

        std::array<PuffinID, rendering::gNumTexturesPerMat> mTexIDs = { 0, 0, 0, 0, 0, 0, 0, 0 };
        std::array<float, rendering::gNumFloatsPerMat> mData = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

        // Array's defined whether material data is overwritten or not
        std::array<bool, rendering::gNumTexturesPerMat> mTexIDOverride;
        std::array<bool, rendering::gNumFloatsPerMat> mDataOverride;
    };
}