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

    static const std::string gMaterialInstanceAssetType = "MaterialInstance";
    static constexpr uint32_t gMaterialInstanceAssetVersion = 1; // Latest version of Material Instance Asset Format

    class MaterialAssetBase : public Asset
    {
    public:

        MaterialAssetBase() : Asset(fs::path()) {}

        explicit MaterialAssetBase(const fs::path& path) : Asset(path) {}

        MaterialAssetBase(const PuffinID id, const fs::path& path) : Asset(id, path) {}

        ~MaterialAssetBase() override = default;

        std::array<int, rendering::gNumTexturesPerMat>& getTexIDs() { return mTexIDs; }
        std::array<float, rendering::gNumFloatsPerMat>& getData() { return mData; }

    protected:

        std::array<int, rendering::gNumTexturesPerMat> mTexIDs = { 0, 0, 0, 0, 0, 0, 0, 0 };
        std::array<float, rendering::gNumFloatsPerMat> mData = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

        void dumpMaterialDataToJson(json& metadata) const;
        void loadMaterialDataFromJson(const json& metadata);

    };

    class MaterialAsset : public MaterialAssetBase
    {
    public:

        MaterialAsset() : MaterialAssetBase(fs::path()) {}

        explicit MaterialAsset(const fs::path& path) : MaterialAssetBase(path) {}

        MaterialAsset(const PuffinID id, const fs::path& path) : MaterialAssetBase(id, path) {}

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

    private:

        PuffinID mVertexShaderID = gInvalidID;
        PuffinID mFragmentShaderID = gInvalidID;

    };

    class MaterialInstanceAsset : public MaterialAssetBase
    {
    public:

        MaterialInstanceAsset() : MaterialAssetBase(fs::path()) {}

        explicit MaterialInstanceAsset(const fs::path& path) : MaterialAssetBase(path) {}

        MaterialInstanceAsset(const PuffinID id, const fs::path& path) : MaterialAssetBase(id, path) {}

        ~MaterialInstanceAsset() override = default;

        [[nodiscard]] const std::string& type() const override
        {
            return gMaterialInstanceAssetType;
        }

        [[nodiscard]] const uint32_t& version() const override
        {
            return gMaterialInstanceAssetVersion;
        }

        bool save();

        bool load();

        void unload() override {}

        [[nodiscard]] PuffinID getBaseMaterialID() const { return mBaseMaterial; }
        void setBaseMaterialID(const PuffinID matID) { mBaseMaterial = matID; }

    private:

        PuffinID mBaseMaterial = gInvalidID; // Base material this instance overrides

        // Array's defined whether material data is overwritten or not
        std::array<bool, rendering::gNumTexturesPerMat> mTexIDsOverride;
        std::array<bool, rendering::gNumFloatsPerMat> mDataOverride;

    };
}