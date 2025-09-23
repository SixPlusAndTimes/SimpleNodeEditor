
#include "NodeEditor.hpp"
#include "imnodes.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "spdlog/spdlog.h"
NodeEditor::NodeEditor()
    : m_nodes(), m_edges(), m_nodeUidGenerator(), m_portUidGenerator(), m_edgeUidGenerator()
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
    ImGui::TextUnformatted("A -- add node");
    ImGui::TextUnformatted("X -- delete selected node or link");
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
            InputPort  inport1(m_portUidGenerator.AllocUniqueID(), 0, "input1");
            InputPort  inport2(m_portUidGenerator.AllocUniqueID(), 1, "input2");
            OutputPort outport1(m_portUidGenerator.AllocUniqueID(), 0, "output1");
                // Node owns their ports
            addNode.AddInputPort(inport1);
            addNode.AddInputPort(inport2);
            addNode.AddOutputPort(outport1);
                // Node editor refer to ports using pointers
            m_inportPorts.emplace(inport1.GetPortUniqueId(),
                                  addNode.GetInputPort(inport1.GetPortUniqueId()));
            m_inportPorts.emplace(inport2.GetPortUniqueId(),
                                  addNode.GetInputPort(inport2.GetPortUniqueId()));
            m_outportPorts.emplace(outport1.GetPortUniqueId(),
                                   addNode.GetOutputPort(outport1.GetPortUniqueId()));

            ImNodes::SetNodeScreenSpacePos(addNode.GetNodeUniqueId(), click_pos);

            const auto& iterRet = m_nodes.insert({addNode.GetNodeUniqueId(), std::move(addNode)});
            if (!iterRet.second)
            {
                SPDLOG_ERROR("insert new node fail! check it out!");
            }
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

void NodeEditor::ShowNodes()
{
    for (auto& [nodeUid, node] : m_nodes)
    {
        ImNodes::BeginNode(nodeUid);
        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted(node.GetNodeTitle().data());
        ImNodes::EndNodeTitleBar();

        size_t min_count = std::min(node.GetInputPorts().size(), node.GetOutputPorts().size());

        for (size_t i = 0; i < min_count; i++)
        {
            // draw input port
            ImNodes::BeginInputAttribute(node.GetInputPorts()[i].GetPortUniqueId());
            ImGui::TextUnformatted("left");
            ImNodes::EndInputAttribute();

            // put output port on the same line
            ImGui::SameLine();

            // draw output port
            ImNodes::BeginOutputAttribute(node.GetOutputPorts()[i].GetPortUniqueId());
            ImGui::TextUnformatted("right");
            ImNodes::EndOutputAttribute();
        }

        if (min_count < node.GetInputPorts().size())
        {
            for (size_t i = min_count; i < node.GetInputPorts().size(); i++)
            {
                ImNodes::BeginInputAttribute(node.GetInputPorts()[i].GetPortUniqueId());
                ImGui::TextUnformatted("left");
                ImNodes::EndInputAttribute();
            }
        }
        else if (min_count < node.GetOutputPorts().size())
        {
            for (size_t i = min_count; i < node.GetOutputPorts().size(); i++)
            {
                ImNodes::BeginOutputAttribute(node.GetOutputPorts()[i].GetPortUniqueId());
                ImGui::TextUnformatted("right");
                ImNodes::EndOutputAttribute();
            }
        }
        ImNodes::EndNode();
    }
}

bool NodeEditor::IsInportAlreadyHasEdge(PortUniqueId portUid)
{
    auto iter = m_inportPorts.find(portUid);
    if (iter != m_inportPorts.end())
    {
        InputPort* inport = iter->second;
        if (inport->GetEdgeUid() != -1) 
        {
            return true;
        }
    }
    else 
    {
        SPDLOG_ERROR("can not find inport, portid = {}, checkit!", portUid);
    }
    return false;
}

void NodeEditor::HandleAddEdges()
{
    PortUniqueId startPortId, endPortId;
    if (ImNodes::IsLinkCreated(&startPortId, &endPortId))
    {
        // avoid multiple edges linking to the same inport
        if (IsInportAlreadyHasEdge(endPortId)) 
        {
            SPDLOG_WARN("inport port can not have multiple edges, inportId = {}", endPortId);
            return ;
        }

        Edge newEdge(startPortId, endPortId, m_edgeUidGenerator.AllocUniqueID());

        // set inportport's edgeid
        if (m_inportPorts.count(endPortId) != 0 && m_inportPorts[endPortId] != nullptr)
        {
            InputPort& inputPort = *m_inportPorts[endPortId];
            inputPort.SetEdgeUid(newEdge.GetEdgeUniqueId());
        }
        else
        {
            SPDLOG_ERROR(
                "not find input port or the pointer is nullptr when handling new edges, check it! "
                "endPortId is{}",
                endPortId);
        }

        // set outportport's edgeid
        if (m_outportPorts.count(startPortId) != 0 && m_outportPorts[startPortId] != nullptr)
        {
            OutputPort& outpurPort = *m_outportPorts[startPortId];
            outpurPort.PushEdge(newEdge.GetEdgeUniqueId());
        }
        else
        {
            SPDLOG_ERROR(
                "not find output port or the pointer is nullptr when handling new edges, check it! "
                "startPortId is {}",
                startPortId);
        }
        m_edges.emplace(newEdge.GetEdgeUniqueId(), std::move(newEdge));
    }
}

void NodeEditor::ShowEdges()
{
    for (const auto& [edgeUid, edge] : m_edges)
    {
        ImNodes::Link(edge.GetEdgeUniqueId(), edge.GetInputPortUid(), edge.GetOutputPortUid());
    }
}

void NodeEditor::DeleteEdgesBeforDeleteNode(NodeUniqueId nodeUid)
{
    if (m_nodes.count(nodeUid) == 0)
    {
        SPDLOG_ERROR("handling an nonexisting node, please check it! nodeUid = {}", nodeUid);
        return;
    }

    Node& node = m_nodes[nodeUid]; // implies that Node has a default constructor
    for (InputPort& inPort : node.GetInputPorts())
    {
        m_edges.erase(inPort.GetEdgeUid());
        inPort.SetEdgeUid(-1);
    }

    for (OutputPort& outPort : node.GetOutputPorts())
    {
        for (EdgeUniqueId edgeUid : outPort.GetEdgeUids())
        {
            m_edges.erase(edgeUid);
        }
        outPort.ClearEdges();
    }
}

void NodeEditor::HandleDeletingNodes()
{
    // erase selected nodes
    const int num_selected_nodes = ImNodes::NumSelectedNodes();
    if (num_selected_nodes > 0 && ImGui::IsKeyReleased(ImGuiKey_X))
    {
        std::vector<int> selected_nodes;
        selected_nodes.resize(num_selected_nodes);
        ImNodes::GetSelectedNodes(selected_nodes.data());
        for (const NodeUniqueId nodeUid : selected_nodes)
        {
            if (m_nodes.count(nodeUid) == 0)
            {
                SPDLOG_ERROR("deleting a nonexisting node, please check it! nodeUid = {}", nodeUid);
                continue;
            }
            // before we erase the node, we need delete the linked edge first
            DeleteEdgesBeforDeleteNode(nodeUid);
            m_nodes.erase(nodeUid);
        }
    }
}

void NodeEditor::HandleDeletingEdges()
{
        // erase selected links
    const int num_selected = ImNodes::NumSelectedLinks();
    if (num_selected > 0 && ImGui::IsKeyReleased(ImGuiKey_X))
    {
        std::vector<int> selected_links;
        selected_links.resize(num_selected);
        ImNodes::GetSelectedLinks(selected_links.data());
        for (const EdgeUniqueId edgeUid : selected_links)
        {
            m_edges.erase(edgeUid);
        }
    }

        // if a link is detached from a pin of node, erase it
    EdgeUniqueId detachedEdgeUId;
    if (ImNodes::IsLinkDestroyed(&detachedEdgeUId)) // is this function usefull? in which situation?
    {
        SPDLOG_ERROR("destroyed link id{}", detachedEdgeUId);
        m_edges.erase(detachedEdgeUId);
    }

    // Edge::EdgeUniqueId dropedEdgeUId;
    // if (ImNodes::IsLinkDropped(&dropedEdgeUId))  // is this api usefull?
    // {
    //     SPDLOG_ERROR("dropped link id{}", dropedEdgeUId);
    // }
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

    HandleDeletingEdges();
    HandleDeletingNodes();

    ImGui::End();
}

void NodeEditor::NodeEditorDestroy() {}