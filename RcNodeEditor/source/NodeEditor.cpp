
#include "NodeEditor.hpp"
#include "imnodes.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "spdlog/spdlog.h"
NodeEditor::NodeEditor()
: m_nodes()
, m_edges()
, m_nodeUidGenerator()
, m_portUidGenerator()
, m_edgeUidGenerator()
{

}
void DebugDrawRect(ImRect rect)
{
    ImDrawList* draw_list    = ImGui::GetWindowDrawList();
    ImU32       border_color = IM_COL32(255, 0, 0, 255); // Example: red border

    float rounding  = 0.0f; // No corner rounding
    float thickness = 1.0f; // Border thickness
    draw_list->AddRect(rect.Min, rect.Max, border_color, rounding, 0, thickness);
}

void NodeEditor::NodeEditorInitialize()
{
    ImNodesIO& io                           = ImNodes::GetIO();
    io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
}

void NodeEditor::ShowMenu()
{
    if (ImGui::BeginMenuBar())
    {
        // ShowMiniMapMenu();
        if (ImGui::BeginMenu("Style"))
        {
            if (ImGui::MenuItem("Classic"))
            {
                ImGui::StyleColorsClassic();
                ImNodes::StyleColorsClassic();
            }
            if (ImGui::MenuItem("Dark"))
            {
                ImGui::StyleColorsDark();
                ImNodes::StyleColorsDark();
            }
            if (ImGui::MenuItem("Light"))
            {
                ImGui::StyleColorsLight();
                ImNodes::StyleColorsLight();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void NodeEditor::ShowInfos()
{
    ImGui::TextUnformatted("Edit the color of the output color window using nodes.");
        // ImGui::Columns(2);
    ImGui::TextUnformatted("A -- add node");
    ImGui::TextUnformatted("X -- delete selected node or link");
        // ImGui::NextColumn();
        // if (ImGui::Checkbox("emulate_three_button_mouse", &emulate_three_button_mouse))
        // {
        //     ImNodes::GetIO().EmulateThreeButtonMouse.Modifier =
        //         emulate_three_button_mouse ? &ImGui::GetIO().KeyAlt : NULL;
        // }
        // ImGui::Columns(1);
}

void NodeEditor::HandleAddNodes()
{
        const bool open_popup = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
                            ImNodes::IsEditorHovered() && ImGui::IsKeyReleased(ImGuiKey_A);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
        if (!ImGui::IsAnyItemHovered() && open_popup)
        {
            ImGui::OpenPopup("add node");
        }

        if (ImGui::BeginPopup("add node"))
        {
            const ImVec2 click_pos = ImGui::GetMousePosOnOpeningCurrentPopup();

            if (ImGui::MenuItem("add"))
            {
                Node addNode(m_nodeUidGenerator.AllocUniqueID(), Node::NodeType::NormalNode, "ADD");
                addNode.AddInputPort({m_portUidGenerator.AllocUniqueID(), 0, "input1"});
                addNode.AddInputPort({m_portUidGenerator.AllocUniqueID(), 1, "input2"});
                addNode.AddOutputPort({m_portUidGenerator.AllocUniqueID(), 0, "output1"});
                const auto& iterRet = m_nodes.insert({addNode.GetNodeUniqueId(), addNode}); // can not move addNode here, cause it will be used in afterwards
                if (!iterRet.second) {
                   SPDLOG_ERROR("insert new node fail! check it out!");
                }

                ImNodes::SetNodeScreenSpacePos(addNode.GetNodeUniqueId(), click_pos);
            }


            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();

}

void NodeEditor::ShowNodes()
{
    for (const auto&[nodeUid, node] : m_nodes)
    {
        ImNodes::BeginNode(nodeUid);
        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted(node.GetNodeTitle().data());
        ImNodes::EndNodeTitleBar();

        for (auto& port : node.GetInputPorts())
        {
            ImNodes::BeginInputAttribute(port.GetPortUniqueId()); // TODO: 
            ImGui::TextUnformatted("left");
            ImNodes::EndInputAttribute();
        }

        // ImGui::SameLine();

        ImGui::Spacing();
        
        for (auto& port : node.GetOutputPorts())
        {
            ImNodes::BeginOutputAttribute(port.GetPortUniqueId()); // TODO: 
            ImGui::TextUnformatted("left");
            ImNodes::EndInputAttribute();
        }

        ImNodes::EndNode();
    }
}

void NodeEditor::HandleAddEdges()
{
            int start_attr, end_attr;
        if (ImNodes::IsLinkCreated(&start_attr, &end_attr))
        {
            // const NodeType start_type = graph_.node(start_attr).type;
            // const NodeType end_type   = graph_.node(end_attr).type;

            // const bool valid_link = start_type != end_type;
            // if (valid_link)
            // {
            //     // Ensure the edge is always directed from the value to
            //     // whatever produces the value
            //     if (start_type != NodeType::value)
            //     {
            //         std::swap(start_attr, end_attr);
            //     }
            //     graph_.insert_edge(start_attr, end_attr);
            // }
            Edge newEdge(start_attr, end_attr, m_edgeUidGenerator.AllocUniqueID());
            m_edges.emplace(newEdge.GetEdgeUniqueId(), std::move(newEdge));
        }

}

void NodeEditor::ShowEdges()
{
    for (const auto&[edgeUid, edge] : m_edges)
    {
        // If edge doesn't start at value, then it's an internal edge, i.e.
        // an edge which links a node's operation to its input. We don't
        // want to render node internals with visible links.
        // if (graph_.node(edge.from).type != NodeType::value) continue;

        ImNodes::Link(edge.GetEdgeUniqueId(), edge.GetInputPortUid(), edge.GetOutputPortUid());
    }
}
void NodeEditor::NodeEditorShow()
{
    // Update timer context
    auto flags = ImGuiWindowFlags_MenuBar;

    // The node editor window
    ImGui::Begin("color node editor", NULL, flags);

    ShowMenu();
    ShowInfos();

    ImNodes::BeginNodeEditor();

    HandleAddNodes();

    ShowNodes();
    ShowEdges();

    // ImNodes::MiniMap(0.2f, minimap_location_);
    ImNodes::EndNodeEditor();

    HandleAddEdges();

    // HandleDeletedEdges();

    ImGui::End();
}

void NodeEditor::NodeEditorDestroy() {}