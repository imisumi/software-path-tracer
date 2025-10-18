#include "SceneHierarchyPanel.h"
#include <imgui.h>

namespace ui
{

SceneHierarchyPanel::SceneHierarchyPanel(std::shared_ptr<render::Scene> scene,
                                         render::NodeID* selected_node_id)
    : m_scene(scene), m_selected_node_id(selected_node_id)
{
}

void SceneHierarchyPanel::render()
{
    if (!m_is_open)
        return;

    ImGui::Begin("Scene Hierarchy", &m_is_open);

    const auto& all_nodes = m_scene->GetAllNodes();
    ImGui::Text("Objects: %zu", all_nodes.size());
    ImGui::Separator();

    // Render all nodes (flat list for now, can add hierarchy later)
    if (ImGui::BeginChild("NodeList", ImVec2(0, 0), ImGuiChildFlags_None))
    {
        for (const auto& [id, node] : all_nodes)
        {
            renderNode(node);
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

void SceneHierarchyPanel::renderNode(render::SceneNode* node)
{
    if (!node)
        return;

    ImGui::PushID(node->GetID());

    // Node icon and name
    const char* icon = getNodeIcon(node->GetType());
    bool is_selected = (m_selected_node_id && *m_selected_node_id == node->GetID());

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
                               ImGuiTreeNodeFlags_SpanAvailWidth;
    if (is_selected)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    std::string label = std::string(icon) + " " + node->GetName();
    ImGui::TreeNodeEx(label.c_str(), flags);

    // Handle selection
    if (ImGui::IsItemClicked())
    {
        *m_selected_node_id = node->GetID();
    }

    // Right-click context menu
    if (ImGui::BeginPopupContextItem())
    {
        ImGui::Text("Object: %s", node->GetName().c_str());
        ImGui::Separator();

        if (ImGui::MenuItem("Focus"))
        {
            // TODO: Focus camera on this object
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Duplicate"))
        {
            // TODO: Duplicate object
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Delete"))
        {
            m_scene->DeleteNode(node->GetID());
            // Deselect if this was selected
            if (m_selected_node_id && *m_selected_node_id == node->GetID())
            {
                *m_selected_node_id = 0;
            }
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::PopID();
}

const char* SceneHierarchyPanel::getNodeIcon(render::NodeType type)
{
    switch (type)
    {
        case render::NodeType::SCENE_ROOT:
            return "[R]";
        case render::NodeType::SPHERE_OBJECT:
            return "[S]";
        case render::NodeType::MATERIAL:
            return "[M]";
        case render::NodeType::GROUP:
            return "[G]";
        default:
            return "[?]";
    }
}

}  // namespace ui
