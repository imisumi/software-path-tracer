#pragma once

#include "IPanel.h"
#include "render/Scene.h"
#include <memory>

namespace ui
{

class SceneHierarchyPanel : public IPanel
{
private:
    std::shared_ptr<render::Scene> m_scene;
    render::NodeID* m_selected_node_id;  // Pointer to shared selection state
    bool m_is_open = true;

public:
    SceneHierarchyPanel(std::shared_ptr<render::Scene> scene, render::NodeID* selected_node_id);

    void render() override;
    const std::string_view getTitle() const override { return "Scene Hierarchy"; }
    bool isOpen() const override { return m_is_open; }
    void setOpen(bool open) override { m_is_open = open; }

private:
    void renderNode(render::SceneNode* node);
    const char* getNodeIcon(render::NodeType type);
};

}  // namespace ui
