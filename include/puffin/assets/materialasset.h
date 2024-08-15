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

        MaterialAsset();
        explicit MaterialAsset(const fs::path& path);
        MaterialAsset(const PuffinID id, const fs::path& path);

        ~MaterialAsset() override = default;

        [[nodiscard]] const std::string& GetType() const override;
        [[nodiscard]] const uint32_t& GetVersion() const override;

        bool Save() override;

        bool Load(bool loadHeaderOnly = false) override;

        void Unload() override {}

        [[nodiscard]] PuffinID GetVertexShaderID() const;
        void SetVertexShaderID(const PuffinID vertID);

        [[nodiscard]] PuffinID GetFragmentShaderID() const;
        void SetFragmentShaderID(const PuffinID fragID);

    private:

        PuffinID mVertexShaderID = gInvalidID;
        PuffinID mFragmentShaderID = gInvalidID;
    };

    class MaterialInstanceAsset : public Asset
    {
    public:

        MaterialInstanceAsset();
        explicit MaterialInstanceAsset(const fs::path& path);
        MaterialInstanceAsset(const PuffinID id, const fs::path& path);

        ~MaterialInstanceAsset() override = default;

        [[nodiscard]] const std::string& GetType() const override;
        [[nodiscard]] const uint32_t& GetVersion() const override;

        bool Save() override;
        bool Load(bool loadHeaderOnly = false) override;
        void Unload() override {}

        [[nodiscard]] PuffinID GetBaseMaterialID() const;
        void SetBaseMaterialID(const PuffinID matID);

        std::array<PuffinID, rendering::gNumTexturesPerMat>& GetTexIDs();
        std::array<float, rendering::gNumFloatsPerMat>& GetData();

    private:

        // Base material this instance overrides. If this is gInvalidID, then it will use engine base material
        PuffinID mBaseMaterial = gInvalidID;

        std::array<PuffinID, rendering::gNumTexturesPerMat> mTexIDs = { 0, 0, 0, 0, 0, 0, 0, 0 };
        std::array<float, rendering::gNumFloatsPerMat> mData = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    };
}