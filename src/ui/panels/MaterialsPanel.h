#pragma once

#include "IPanel.h"
#include "render/Scene.h"
#include <memory>

namespace ui
{

class MaterialsPanel : public IPanel
{
private:
    std::shared_ptr<render::Scene> m_scene;
    render::MaterialDescriptor::Handle* m_selected_material_handle;  // Pointer to App's selection
    bool m_is_open = true;

    // UI state
    char m_new_material_name[128] = "";
    int m_material_type_selection = 0;

    // Color picker for new materials
    float m_new_material_color[3] = {0.8f, 0.8f, 0.8f};

public:
    MaterialsPanel(std::shared_ptr<render::Scene> scene,
                   render::MaterialDescriptor::Handle* selected_material);

    void render() override;
    const std::string_view getTitle() const override { return "Materials"; }
    bool isOpen() const override { return m_is_open; }
    void setOpen(bool open) override { m_is_open = open; }

private:
    void renderMaterialList();
    void renderCreateMaterialSection();
    void renderMaterialCard(const std::string& name, const render::MaterialDescriptor& mat, bool is_selected);
    const char* getMaterialTypeName(render::MaterialType type);
    std::string truncateText(const std::string& text, float max_width);
};

}  // namespace ui
