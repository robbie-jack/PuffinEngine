#pragma once

#include "puffin/assets/asset.h"

#include "puffin/types/uuid.h"
#include "puffin/rendering/materialglobals.h"
#include "nlohmann/json.hpp"

#include <bitset>

using json = nlohmann::json;


namespace puffin::assets
{
    static const std::string gMaterialAssetType = "Material";
    static constexpr uint32_t gMaterialAssetVersion = 1; // Latest version of Material Asset Format

    static const std::string gMaterialInstAssetType = "MaterialInstance";
    static constexpr uint32_t gMaterialInstAssetVersion = 1; // Latest version of Material Instance Asset Format

    class MaterialAsset : public Asset
    {
    public:

        MaterialAsset() : Asset(fs::path()) {}

        explicit MaterialAsset(const fs::path& path) : Asset(path) {}

        MaterialAsset(const PuffinID id, const fs::path& path) : Asset(id, path) {}

        ~MaterialAsset() override = default;

        [[nodiscard]] const std::string& GetType() const override
        {
            return gMaterialAssetType;
        }

        [[nodiscard]] const uint32_t& GetVersion() const override
        {
            return gMaterialAssetVersion;
        }

        bool Save() override;

        bool Load(bool loadHeaderOnly = false) override;

        void Unload() override {}

        [[nodiscard]] PuffinID getVertexShaderID() const { return mVertexShaderID; }
        void setVertexShaderID(const PuffinID vertID) { mVertexShaderID = vertID; }

        [[nodiscard]] PuffinID getFragmentShaderID() const { return mFragmentShaderID; }
        void setFragmentShaderID(const PuffinID fragID) { mFragmentShaderID = fragID; }

    private:

        PuffinID mVertexShaderID = gInvalidID;
        PuffinID mFragmentShaderID = gInvalidID;
    };

    class MaterialInstanceAsset : public Asset
    {
    public:

        MaterialInstanceAsset() : Asset(fs::path()) {}

        explicit MaterialInstanceAsset(const fs::path& path) : Asset(path) {}

        MaterialInstanceAsset(const PuffinID id, const fs::path& path) : Asset(id, path) {}

        ~MaterialInstanceAsset() override = default;

        [[nodiscard]] const std::string& GetType() const override
        {
            return gMaterialInstAssetType;
        }

        [[nodiscard]] const uint32_t& GetVersion() const override
        {
            return gMaterialInstAssetVersion;
        }

        bool Save() override;

        bool Load(bool loadHeaderOnly = false) override;

        void Unload() override {}

        [[nodiscard]] PuffinID getBaseMaterialID() const { return mBaseMaterial; }
        void setBaseMaterialID(const PuffinID matID) { mBaseMaterial = matID; }

        std::array<PuffinID, rendering::gNumTexturesPerMat>& getTexIDs() { return mTexIDs; }
        std::array<float, rendering::gNumFloatsPerMat>& getData() { return mData; }

    private:

        // Base material this instance overrides. If this is gInvalidID, then it will use engine base material
        PuffinID mBaseMaterial = gInvalidID;

        std::array<PuffinID, rendering::gNumTexturesPerMat> mTexIDs = { 0, 0, 0, 0, 0, 0, 0, 0 };
        std::array<float, rendering::gNumFloatsPerMat> mData = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    };
}