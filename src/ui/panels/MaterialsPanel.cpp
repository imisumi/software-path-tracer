#include "MaterialsPanel.h"

#include <imgui.h>

#include <unordered_set>

namespace ui
{

MaterialsPanel::MaterialsPanel(std::shared_ptr<render::Scene> scene,
                               render::MaterialDescriptor::Handle* selected_material)
    : m_scene(scene), m_selected_material_handle(selected_material)
{
}

void MaterialsPanel::render()
{
    if (!m_is_open)
        return;

    ImGui::Begin("Materials", &m_is_open);

    ImGui::Text("Material Library");
    ImGui::Separator();

    renderMaterialList();

    ImGui::Separator();
    renderCreateMaterialSection();

    ImGui::End();
}

void MaterialsPanel::renderMaterialList()
{
    const auto& mat_lib = m_scene->getMaterialLibrary();
    auto named_materials = mat_lib.getNamedMaterials();

    ImGui::Text("Materials: %zu", mat_lib.getMaterialCount());
    ImGui::Separator();

    if (ImGui::BeginChild("MaterialGrid", ImVec2(0, -100), ImGuiChildFlags_Border))
    {
        // Card dimensions
        const float card_size = 100.0f;
        const float card_spacing = 10.0f;
        const float available_width = ImGui::GetContentRegionAvail().x;
        const int cards_per_row =
            std::max(1, (int)((available_width + card_spacing) / (card_size + card_spacing)));

        // Collect all materials from nodes (these are the actual materials in use)
        std::vector<std::pair<std::string, render::MaterialDescriptor::Handle>> all_materials;

        // First add named materials
        for (const auto& name : named_materials)
        {
            auto mat = mat_lib.get(name);
            if (mat)
            {
                all_materials.push_back({name, mat});
            }
        }

        // Then add materials from nodes (auto-named materials)
        const auto& all_nodes = m_scene->GetAllNodes();
        std::unordered_set<render::MaterialDescriptor::Handle> added_materials;

        // Track which materials are already added as named materials
        for (const auto& [name, mat] : all_materials)
        {
            added_materials.insert(mat);
        }

        // Add auto-named materials
        for (const auto& [id, node] : all_nodes)
        {
            auto mat = m_scene->getMaterial(id);
            if (mat && added_materials.find(mat) == added_materials.end())
            {
                // Get the auto-generated name from the library
                std::string auto_name = mat_lib.getAutoName(mat);
                if (!auto_name.empty())
                {
                    all_materials.push_back({auto_name, mat});
                    added_materials.insert(mat);
                }
            }
        }

        // Render cards in grid
        int card_index = 0;
        for (const auto& [name, mat] : all_materials)
        {
            ImGui::PushID(card_index);

            bool is_selected = (m_selected_material_handle && *m_selected_material_handle == mat);
            renderMaterialCard(name, *mat, is_selected);

            // Handle clicking on the card
            if (ImGui::IsItemClicked())
            {
                *m_selected_material_handle = mat;
            }

            // Grid layout
            if ((card_index + 1) % cards_per_row != 0)
            {
                ImGui::SameLine();
            }

            ImGui::PopID();
            card_index++;
        }
    }
    ImGui::EndChild();
}

void MaterialsPanel::renderCreateMaterialSection()
{
    ImGui::Text("Create New Material");

    ImGui::InputText("Name", m_new_material_name, sizeof(m_new_material_name));

    const char* material_types[] = {"Diffuse"};
    ImGui::Combo("Type", &m_material_type_selection, material_types, 1);

    // Color picker for initial albedo
    ImGui::ColorEdit3("Color", m_new_material_color);

    if (ImGui::Button("Create Material", ImVec2(-1, 0)))
    {
        if (m_new_material_name[0] != '\0')
        {
            // Check if name already exists
            if (m_scene->hasNamedMaterial(m_new_material_name))
            {
                // TODO: Show error message or auto-rename
                ImGui::OpenPopup("Material Already Exists");
            }
            else
            {
                render::MaterialDescriptor new_mat;
                new_mat.type = static_cast<render::MaterialType>(m_material_type_selection);

                // Set default parameters based on type
                switch (new_mat.type)
                {
                    case render::MaterialType::DIFFUSE:
                        new_mat.parameters["albedo"] =
                            glm::vec3(m_new_material_color[0], m_new_material_color[1],
                                      m_new_material_color[2]);
                        break;
                        // Future material types will go here
                }

                m_scene->registerNamedMaterial(m_new_material_name, new_mat);

                // Auto-select the newly created material
                *m_selected_material_handle = m_scene->getNamedMaterial(m_new_material_name);

                // Clear input
                m_new_material_name[0] = '\0';
            }
        }
    }

    // Error popup
    if (ImGui::BeginPopupModal("Material Already Exists", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("A material with this name already exists!");
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void MaterialsPanel::renderMaterialCard(const std::string& name,
                                        const render::MaterialDescriptor& mat, bool is_selected)
{
    const float card_size = 100.0f;
    const float text_height = ImGui::GetTextLineHeight();

    ImVec2 card_pos = ImGui::GetCursorScreenPos();
    ImVec2 card_min = card_pos;
    ImVec2 card_max = ImVec2(card_pos.x + card_size, card_pos.y + card_size + text_height + 8);

    // Get color from material
    glm::vec3 mat_color = mat.get("albedo", glm::vec3(0.5f));
    ImVec4 color = ImVec4(mat_color.r, mat_color.g, mat_color.b, 1.0f);

    // Draw selection border
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    if (is_selected)
    {
        draw_list->AddRect(card_min, card_max, IM_COL32(255, 255, 255, 255), 4.0f, 0, 3.0f);
    }

    // Draw color preview square
    ImVec2 color_min = ImVec2(card_pos.x + 2, card_pos.y + 2);
    ImVec2 color_max = ImVec2(card_pos.x + card_size - 2, card_pos.y + card_size - 2);
    draw_list->AddRectFilled(color_min, color_max, ImGui::ColorConvertFloat4ToU32(color), 4.0f);

    // Add subtle border around color square
    draw_list->AddRect(color_min, color_max, IM_COL32(80, 80, 80, 255), 4.0f, 0, 1.0f);

    // Truncate text if needed
    std::string display_name = truncateText(name, card_size - 8);

    // Draw text below the square (centered)
    ImVec2 text_size = ImGui::CalcTextSize(display_name.c_str());
    ImVec2 text_pos =
        ImVec2(card_pos.x + (card_size - text_size.x) * 0.5f, card_pos.y + card_size + 4);
    draw_list->AddText(text_pos, IM_COL32(200, 200, 200, 255), display_name.c_str());

    // Invisible button for interaction
    ImGui::InvisibleButton("##card", ImVec2(card_size, card_size + text_height + 8));

    // Tooltip with full info on hover
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("%s", name.c_str());
        ImGui::Separator();
        ImGui::Text("Type: %s", getMaterialTypeName(mat.type));
        ImGui::Text("RGB: (%.2f, %.2f, %.2f)", mat_color.r, mat_color.g, mat_color.b);
        ImGui::EndTooltip();
    }

    // Right-click context menu
    if (ImGui::BeginPopupContextItem("card_context"))
    {
        ImGui::Text("Material: %s", name.c_str());
        ImGui::Separator();

        if (ImGui::MenuItem("Duplicate"))
        {
            std::string copy_name = name + "_copy";
            m_scene->registerNamedMaterial(copy_name, mat);
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Rename"))
        {
            // TODO: Implement rename dialog
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

std::string MaterialsPanel::truncateText(const std::string& text, float max_width)
{
    const char* ellipsis = "...";
    ImVec2 text_size = ImGui::CalcTextSize(text.c_str());

    if (text_size.x <= max_width)
    {
        return text;
    }

    // Binary search for the right length
    std::string truncated = text;
    while (truncated.length() > 0)
    {
        truncated = text.substr(0, truncated.length() - 1);
        std::string with_ellipsis = truncated + ellipsis;
        ImVec2 size = ImGui::CalcTextSize(with_ellipsis.c_str());
        if (size.x <= max_width)
        {
            return with_ellipsis;
        }
    }

    return ellipsis;
}

const char* MaterialsPanel::getMaterialTypeName(render::MaterialType type)
{
    switch (type)
    {
        case render::MaterialType::DIFFUSE:
            return "Diffuse";
        default:
            return "Unknown";
    }
}

}  // namespace ui
