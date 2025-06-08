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
        MaterialAsset(const UUID id, const fs::path& path);

        ~MaterialAsset() override = default;

        [[nodiscard]] const std::string& GetType() const override;
        [[nodiscard]] const uint32_t& GetVersion() const override;

        bool Save() override;

        bool Load(bool loadHeaderOnly = false) override;

        void Unload() override {}

        [[nodiscard]] UUID GetVertexShaderID() const;
        void SetVertexShaderID(const UUID vertID);

        [[nodiscard]] UUID GetFragmentShaderID() const;
        void SetFragmentShaderID(const UUID fragID);

    private:

        UUID mVertexShaderID = gInvalidID;
        UUID mFragmentShaderID = gInvalidID;
    };

    class MaterialInstanceAsset : public Asset
    {
    public:

        MaterialInstanceAsset();
        explicit MaterialInstanceAsset(const fs::path& path);
        MaterialInstanceAsset(const UUID id, const fs::path& path);

        ~MaterialInstanceAsset() override = default;

        [[nodiscard]] const std::string& GetType() const override;
        [[nodiscard]] const uint32_t& GetVersion() const override;

        bool Save() override;
        bool Load(bool loadHeaderOnly = false) override;
        void Unload() override {}

        [[nodiscard]] UUID GetBaseMaterialID() const;
        void SetBaseMaterialID(const UUID matID);

        std::array<UUID, rendering::gNumTexturesPerMat>& GetTexIDs();
        std::array<float, rendering::gNumFloatsPerMat>& GetData();

    private:

        // Base material this instance overrides. If this is gInvalidID, then it will use engine base material
        UUID mBaseMaterial = gInvalidID;

        std::array<UUID, rendering::gNumTexturesPerMat> mTexIDs = { 0, 0, 0, 0, 0, 0, 0, 0 };
        std::array<float, rendering::gNumFloatsPerMat> mData = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    };
}