#pragma once

#ifndef UI_WINDOW_ENTITY_PROPERTIES_H
#define UI_WINDOW_ENTITY_PROPERTIES_H

#include "UIWindow.h"
//#include "Entity.h"
#include "ECS.h"
#include "imgui/imfilebrowser.h"

namespace Puffin
{
    namespace UI
    {
        class UIWindowEntityProperties : public UIWindow
        {
		public:

			bool Draw(float dt, Puffin::Input::InputManager* InputManager) override;

            inline void SetEntity(ECS::Entity entity_) { entity = entity_; };
            inline void SetWorld(ECS::World* world_) { world = world_; };
            inline void SetFileBrowser(ImGui::FileBrowser* fileDialog_) { fileDialog = fileDialog_; };

            inline bool HasSceneChanged() { return sceneChanged; };

        private:
            ECS::Entity entity;
            ECS::World* world;
            ImGui::FileBrowser* fileDialog;

            bool modelSelected = false;
            bool textureSelected = false;
            bool positionChanged = false;
            bool sceneChanged = false;
        };
    }
}

#endif // UI_WINDOW_ENTITY_PROPERTIES_H