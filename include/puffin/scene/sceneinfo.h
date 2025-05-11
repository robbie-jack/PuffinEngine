#pragma once

#include <nlohmann/json.hpp>

namespace puffin
{
	namespace scene
	{
		enum class SceneType
		{
			Invalid,
			Scene2D,
			Scene3D
		};

		inline std::string SceneTypeToString(SceneType sceneType)
		{
			switch(sceneType)
			{
			case SceneType::Scene2D: return "Scene2D";
			case SceneType::Scene3D: return "Scene3D";
			default: return "";
			}
		}

		inline SceneType StringToSceneType(const std::string& string)
		{
			if (strcmp("Scene2D", string.c_str()) == 0)
				return SceneType::Scene2D;

			if (strcmp("Scene3D", string.c_str()) == 0)
				return SceneType::Scene3D;

			return SceneType::Invalid;
		}

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
