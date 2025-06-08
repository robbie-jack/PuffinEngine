#pragma once

#include <nlohmann/json.hpp>

#include "types/scene_type.h"

namespace puffin
{
	namespace scene
	{
		struct SceneInfo
		{
			SceneType sceneType = SceneType::Invalid;
		};

		inline nlohmann::json SerializeSceneInfo(const SceneInfo& sceneInfo)
		{
			nlohmann::json json;
			json["sceneType"] = SceneTypeToString(sceneInfo.sceneType);
			return json;
		}

		inline SceneInfo DeserializeSceneInfo(const nlohmann::json& json)
		{
			SceneInfo sceneInfo;
			sceneInfo.sceneType = StringToSceneType(json["sceneType"]);
			return sceneInfo;
		}
	}
}
