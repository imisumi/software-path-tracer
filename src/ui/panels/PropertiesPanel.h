#pragma once

#include "IPanel.h"
#include "render/Scene.h"
#include <memory>

namespace ui
{

class PropertiesPanel : public IPanel
{
private:
    std::shared_ptr<render::Scene> m_scene;
    render::NodeID* m_selected_node_id;  // Pointer to shared selection state
    bool m_is_open = true;

    // Temporary edit buffers
    char m_name_buffer[128] = "";
    float m_position[3] = {0, 0, 0};
    float m_radius = 1.0f;

public:
    PropertiesPanel(std::shared_ptr<render::Scene> scene, render::NodeID* selected_node_id);

    void render() override;
    const std::string_view getTitle() const override { return "Properties"; }
    bool isOpen() const override { return m_is_open; }
    void setOpen(bool open) override { m_is_open = open; }

private:
    void renderTransformProperties(render::SceneNode* node);
    void renderSphereProperties(render::SphereObject* sphere);
    void renderMaterialAssignment(render::SceneNode* node);
};

}  // namespace ui
