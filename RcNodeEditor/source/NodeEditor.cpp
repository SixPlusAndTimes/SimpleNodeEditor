#include "NodeEditor.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "spdlog/spdlog.h"
#include <cstdint>

struct NodeDescription
{
    std::string              m_nodeName;
    std::vector<std::string> m_inportNames;
    std::vector<std::string> m_outportNames;
};

static std::unordered_map<std::string, NodeDescription> s_nodeDescriptions;

NodeEditor::NodeEditor()
    : m_nodes(),
      m_edges(),
      m_inportPorts(),
      m_outportPorts(),
      m_nodeUidGenerator(),
      m_portUidGenerator(),
      m_edgeUidGenerator(),
      m_minimap_location(ImNodesMiniMapLocation_BottomRight)
{
    // TODO : this add operation should be placed
    s_nodeDescriptions["ADD"] = NodeDescription("ADD", {"inport1", "inport2"}, {"outport2"});

    s_nodeDescriptions["select"] =
        NodeDescription("select",
                        {"inport1", "inport2", "inport3", "inport4", "inport5", "inport6",
                         "inport7", "inport8", "inport9", "inport10", "inport11", "inport12"},
                        {"outport1", "outport2", "outport3", "outport4", "outport5"});

    s_nodeDescriptions["ggg"] = NodeDescription{"ggg", {"inport1", "inport2", "inport3"}, {"outport1", "outport2","outport3", "outport4", "outport5"}};

    s_nodeDescriptions["ar"] = NodeDescription{"ar", {"inport1", "inport2", "inport3"}, {"outport1", "outport2", "outport3"}};

    s_nodeDescriptions["bb"] = NodeDescription{"bb", {"inport1", "inport2", "inport3"}, {"outport1", "outport2", "outport3"}};
    s_nodeDescriptions["aa"] = NodeDescription{"aa", {"inport1", "inport2", "inport3"}, {"outport1", "outport2", "outport3"}};
    s_nodeDescriptions["cc"] = NodeDescription{"cc", {"inport1", "inport2", "inport3"}, {"outport1", "outport2", "outport3"}};
    
    s_nodeDescriptions["sim"] = NodeDescription{"sim", {"inport1", "inport2"}, {"outport1", "outport2"}};

    s_nodeDescriptions["cp"] = NodeDescription{"cp", {"inport1", "inport2", "inport3"}, {"outport1"}};

    s_nodeDescriptions["pp"] = NodeDescription{"pp", {"inport1"}, {"outport1"}};

    s_nodeDescriptions["sourcenode"] =
        NodeDescription{"sourcenode",
                        {},
                        {"outport1", "outport2", "outport3", "outport4", "outport5", "outport6",
                         "outport7", "outport8", "outport9", "outport10", "outport11", "outport12"}};

    s_nodeDescriptions["dstnode"] = NodeDescription{"dstnode", {"inport"}, {}};
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

        for (const auto& [nodeName, nodeDescription] : s_nodeDescriptions)
        {
            if (ImGui::MenuItem(nodeName.c_str()))
            {
                Node newNode(m_nodeUidGenerator.AllocUniqueID(), Node::NodeType::NormalNode,
                             nodeName);
                // we must reserve the vector first; if not , the reallocation of std::vector will
                // mess memory up
                newNode.GetInputPorts().reserve(nodeDescription.m_inportNames.size());
                newNode.GetOutputPorts().reserve(nodeDescription.m_outportNames.size());

                for (int index = 0; index < nodeDescription.m_inportNames.size(); ++index)
                {
                    InputPort newInport(m_portUidGenerator.AllocUniqueID(), index,
                                        nodeDescription.m_inportNames[index]);
                    newNode.AddInputPort(newInport);
                    m_inportPorts.emplace(newInport.GetPortUniqueId(),
                                          newNode.GetInputPort(newInport.GetPortUniqueId()));
                }

                for (int index = 0; index < nodeDescription.m_outportNames.size(); ++index)
                {
                    OutputPort newOutport(m_portUidGenerator.AllocUniqueID(), index,
                                          nodeDescription.m_outportNames[index]);
                    newNode.AddOutputPort(newOutport);
                    m_outportPorts.emplace(newOutport.GetPortUniqueId(),
                                           newNode.GetOutputPort(newOutport.GetPortUniqueId()));
                }

                ImNodes::SetNodeScreenSpacePos(newNode.GetNodeUniqueId(), click_pos);

                const auto& iterRet =
                    m_nodes.insert({newNode.GetNodeUniqueId(), std::move(newNode)});
                if (!iterRet.second)
                {
                    SPDLOG_ERROR("insert new node fail! check it out!");
                }
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
            ImGui::TextUnformatted(node.GetInputPorts()[i].GetPortname().data());
            ImNodes::EndInputAttribute();

            // put output port on the same line
            ImGui::SameLine();

            // draw output port
            ImNodes::BeginOutputAttribute(node.GetOutputPorts()[i].GetPortUniqueId());
            ImGui::TextUnformatted(node.GetOutputPorts()[i].GetPortname().data());
            ImNodes::EndOutputAttribute();
        }

        if (min_count < node.GetInputPorts().size())
        {
            for (size_t i = min_count; i < node.GetInputPorts().size(); i++)
            {
                ImNodes::BeginInputAttribute(node.GetInputPorts()[i].GetPortUniqueId());
                ImGui::TextUnformatted(node.GetInputPorts()[i].GetPortname().data());
                ImNodes::EndInputAttribute();
            }
        }
        else if (min_count < node.GetOutputPorts().size())
        {
            for (size_t i = min_count; i < node.GetOutputPorts().size(); i++)
            {
                ImNodes::BeginOutputAttribute(node.GetOutputPorts()[i].GetPortUniqueId());
                ImGui::TextUnformatted(node.GetOutputPorts()[i].GetPortname().data());
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
            SPDLOG_INFO("inportportid = {}, edgeUid = {}", portUid, inport->GetEdgeUid());
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
            SPDLOG_WARN("inport port can not have multiple edges, inportUid = {}", endPortId);
            return;
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

void NodeEditor::DeleteEdgeUidFromPort(EdgeUniqueId edgeUid)
{
    auto iterEdge = m_edges.find(edgeUid);
    if (iterEdge != m_edges.end())
    {
        Edge& edge = iterEdge->second;
        if (m_outportPorts.count(edge.GetInputPortUid()))
        {
            m_outportPorts[edge.GetInputPortUid()]->DeletEdge(edgeUid);
        }
        else
        {
            SPDLOG_ERROR("cannot find inports in m_outportPorts inportUid = {}",
                         edge.GetInputPortUid());
        }

        if (m_inportPorts.count(edge.GetOutputPortUid()))
        {
            m_inportPorts[edge.GetOutputPortUid()]->SetEdgeUid(-1);
        }
        else
        {
            SPDLOG_ERROR("cannot find inports in m_inportPorts outputid = {}",
                         edge.GetOutputPortUid());
        }
    }
    else
    {
        SPDLOG_ERROR("cannot find edge in m_edges, edgeUid = {}, check it!", edgeUid);
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
            if (m_edges.count(edgeUid) != 0)
            {
                DeleteEdgeUidFromPort(edgeUid);
                m_edges.erase(edgeUid);
            }
            else
            {
                SPDLOG_ERROR("cannot find edge in m_edges, edgeUid = {}, check it!", edgeUid);
            }
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

    // ImNodes::GetIO().EmulateThreeButtonMouse.Modifier = &ImGui::GetIO().KeyAlt;

    ImNodes::BeginNodeEditor();

    HandleAddNodes();
    ShowNodes();
    ShowEdges();

    ImNodes::MiniMap(0.2f, m_minimap_location);

    // ImNodes::MiniMap(0.2f, minimap_location_);
    ImNodes::EndNodeEditor();

    HandleAddEdges();

    HandleDeletingEdges();
    HandleDeletingNodes();

    ImGui::End();
}

void NodeEditor::NodeEditorDestroy() {}