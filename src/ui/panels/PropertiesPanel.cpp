#include "PropertiesPanel.h"
#include <imgui.h>
#include <cstring>

namespace ui
{

PropertiesPanel::PropertiesPanel(std::shared_ptr<render::Scene> scene,
                                 render::NodeID* selected_node_id)
    : m_scene(scene), m_selected_node_id(selected_node_id)
{
}

void PropertiesPanel::render()
{
    if (!m_is_open)
        return;

    ImGui::Begin("Properties", &m_is_open);

    if (!m_selected_node_id || *m_selected_node_id == 0)
    {
        ImGui::TextDisabled("No object selected");
        ImGui::Text("Select an object from the Scene Hierarchy");
        ImGui::End();
        return;
    }

    auto node = m_scene->FindNode(*m_selected_node_id);
    if (!node)
    {
        ImGui::TextDisabled("Selected object not found");
        ImGui::End();
        return;
    }

    // Object name
    ImGui::PushItemWidth(-1);
    std::strncpy(m_name_buffer, node->GetName().c_str(), sizeof(m_name_buffer) - 1);
    if (ImGui::InputText("##Name", m_name_buffer, sizeof(m_name_buffer)))
    {
        node->SetName(m_name_buffer);
    }
    ImGui::PopItemWidth();

    ImGui::Separator();

    // Transform properties
    renderTransformProperties(node);

    ImGui::Separator();

    // Type-specific properties
    switch (node->GetType())
    {
        case render::NodeType::SPHERE_OBJECT:
            if (auto sphere = dynamic_cast<render::SphereObject*>(node))
            {
                renderSphereProperties(sphere);
            }
            break;
        default:
            break;
    }

    ImGui::Separator();

    // Material assignment
    renderMaterialAssignment(node);

    ImGui::End();
}

void PropertiesPanel::renderTransformProperties(render::SceneNode* node)
{
    ImGui::Text("Transform");

    // Position
    auto pos = node->GetPosition();
    m_position[0] = pos.x;
    m_position[1] = pos.y;
    m_position[2] = pos.z;

    if (ImGui::DragFloat3("Position", m_position, 0.1f))
    {
        node->SetPosition(glm::vec3(m_position[0], m_position[1], m_position[2]));
        m_scene->markDirty();
    }

    // TODO: Add rotation and scale when implemented in SceneNode
}

void PropertiesPanel::renderSphereProperties(render::SphereObject* sphere)
{
    ImGui::Text("Sphere Properties");

    m_radius = sphere->GetRadius();
    if (ImGui::DragFloat("Radius", &m_radius, 0.1f, 0.01f, 1000.0f))
    {
        sphere->SetRadius(m_radius);
        m_scene->markDirty();
    }
}

void PropertiesPanel::renderMaterialAssignment(render::SceneNode* node)
{
    ImGui::Text("Material");

    auto current_material = m_scene->getMaterial(node->GetID());

    if (current_material)
    {
        // Show current material info
        auto& mat_lib = m_scene->getMaterialLibrary();
        std::string mat_name = mat_lib.getAutoName(current_material);

        // Check if it has a custom name
        auto named_materials = mat_lib.getNamedMaterials();
        for (const auto& name : named_materials)
        {
            if (mat_lib.get(name) == current_material)
            {
                mat_name = name;
                break;
            }
        }

        ImGui::Text("Current: %s", mat_name.c_str());

        // Show color preview
        glm::vec3 albedo = current_material->get("albedo", glm::vec3(0.5f));
        ImVec4 color = ImVec4(albedo.r, albedo.g, albedo.b, 1.0f);
        ImGui::ColorButton("##MaterialPreview", color,
                          ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker,
                          ImVec2(40, 40));
        ImGui::SameLine();
        ImGui::Text("Albedo: (%.2f, %.2f, %.2f)", albedo.r, albedo.g, albedo.b);

        if (ImGui::Button("Remove Material"))
        {
            m_scene->removeMaterial(node->GetID());
            m_scene->markDirty();
        }
    }
    else
    {
        ImGui::TextDisabled("No material assigned");

        if (ImGui::Button("Assign Material"))
        {
            // TODO: Open material picker dialog
            // For now, create a default material
            render::MaterialDescriptor default_mat;
            default_mat.type = render::MaterialType::DIFFUSE;
            default_mat.parameters["albedo"] = glm::vec3(0.8f, 0.8f, 0.8f);
            m_scene->setMaterial(node->GetID(), default_mat);
            m_scene->markDirty();
        }
    }
}

}  // namespace ui
