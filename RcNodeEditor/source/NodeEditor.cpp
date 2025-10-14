#include "NodeEditor.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "spdlog/spdlog.h"
#include <cstdint>
#include "Helpers.h"
#include "YamlParser.hpp"


namespace SimpleNodeEditor
{
    
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
    // TODO: file path may be a constant value or configed in Config.yaml?
    NodeDescriptionParser nodeParser("./resource/NodeDescriptions.yaml");
    std::vector<NodeDescription> nodeDescriptions =  nodeParser.ParseNodeDescriptions();
    for (const auto& nodeD : nodeDescriptions)
    {
        s_nodeDescriptions.emplace(nodeD.m_nodeName, std::move(nodeD));
    }

    std::string pipeLineYamlFileName = "./resource/testpipelines/pipelinetest1.yaml";
    PipelineParser pipeLineParser;
    if (pipeLineParser.LoadFile(pipeLineYamlFileName))
    {
        auto ret = pipeLineParser.ParseNodes();
    }
    else
    {
        SPDLOG_ERROR("pipelineparser loadfile failed, filename is [{}], check it!", pipeLineYamlFileName);
    }

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

    static std::string previewSelected = s_nodeDescriptions.begin()->first;

    if (ImGui::BeginPopup("add node"))
    {
        const ImVec2 click_pos = ImGui::GetMousePosOnOpeningCurrentPopup();

        ImGuiComboFlags flags = 0 ;
        bool is_selected = false;

        if (ImGui::BeginCombo("add node now", previewSelected.c_str(), flags))
        {
            static ImGuiTextFilter filter;
            if (ImGui::IsWindowAppearing())
            {
                ImGui::SetKeyboardFocusHere();
                filter.Clear();
            }
            filter.Draw("##Filter", -FLT_MIN);

            ImGui::SetKeyboardFocusHere(-1);

            for (auto& [nodeName, nodeDesc] : s_nodeDescriptions)
            {
                if (filter.PassFilter(nodeName.c_str()))
                {
                    ImGui::Selectable(nodeName.c_str());
                    if (ImGui::IsItemActive() && ImGui::IsItemClicked())
                    {
                        SPDLOG_INFO("User selected {}, will be added in canvas", nodeName);
                        previewSelected = nodeName;
                        is_selected = true;
                        ImGui::CloseCurrentPopup(); // close Combo right now
                    }
                }
            }
            ImGui::EndCombo();
        }

        
        if (is_selected)
        {
            ImGui::CloseCurrentPopup(); // close popup right now

            NodeDescription selectedNodeDescription = s_nodeDescriptions.at(previewSelected);
            Node newNode(m_nodeUidGenerator.AllocUniqueID(), Node::NodeType::NormalNode,
                             selectedNodeDescription.m_nodeName);
                // we must reserve the vector first; if not , the reallocation of std::vector will
                // mess memory up
                newNode.GetInputPorts().reserve(selectedNodeDescription.m_inputPortNames.size());
                newNode.GetOutputPorts().reserve(selectedNodeDescription.m_outputPortNames.size());

                for (int index = 0; index < selectedNodeDescription.m_inputPortNames.size(); ++index)
                {
                    InputPort newInport(m_portUidGenerator.AllocUniqueID(), index,
                                        selectedNodeDescription.m_inputPortNames[index], newNode.GetNodeUniqueId());
                    newNode.AddInputPort(newInport);
                    m_inportPorts.emplace(newInport.GetPortUniqueId(),
                                          newNode.GetInputPort(newInport.GetPortUniqueId()));
                }

                for (int index = 0; index < selectedNodeDescription.m_outputPortNames.size(); ++index)
                {
                    OutputPort newOutport(m_portUidGenerator.AllocUniqueID(), index,
                                          selectedNodeDescription.m_outputPortNames[index], newNode.GetNodeUniqueId());
                    newNode.AddOutputPort(newOutport);
                    m_outportPorts.emplace(newOutport.GetPortUniqueId(),
                                           newNode.GetOutputPort(newOutport.GetPortUniqueId()));
                }
                
                ImNodes::SetNodeScreenSpacePos(newNode.GetNodeUniqueId(), click_pos);
                // add logs to understand the 3 types of coordinates
                ImVec2 newNodeGridSpace  = ImNodes::GetNodeGridSpacePos(newNode.GetNodeUniqueId()); // imnode internal datastructure : ImNodeData::Origin , store the coordinates in grid space. The value of this coodinate may be negative or posive, and changed with editor panning events.
                ImVec2 newNodeScreenSpace  = ImNodes::GetNodeScreenSpacePos(newNode.GetNodeUniqueId());  // defined by app window. the value of this coordinate is definately positive.
                ImVec2 newNodeEditorSpace  = ImNodes::GetNodeEditorSpacePos(newNode.GetNodeUniqueId()); // just like a viewport coordination? the value of this coordinate is definetely positive .

                SPDLOG_INFO("add new node, nodeuid = {}, girdspace = [{},{}], screenspace = [{},{}], editorspace = [{}, {}]", newNode.GetNodeUniqueId(), 
                    newNodeGridSpace.x, newNodeGridSpace.y,
                    newNodeScreenSpace.x, newNodeScreenSpace.y, 
                    newNodeEditorSpace.x, newNodeEditorSpace.y);
                if (!m_nodes.insert({newNode.GetNodeUniqueId(), std::move(newNode)}).second)
                {
                    SPDLOG_ERROR("insert new node fail! check it!");
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
            newEdge.SetDestinationNodeUid(inputPort.OwnedByNodeUid());
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
            newEdge.SetSourceNodeUid(outpurPort.OwnedByNodeUid());
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
        ImNodes::Link(edgeUid, edge.GetSourcePortUid(), edge.GetDestinationPortUid());
    }
}

void NodeEditor::DeleteEdgesBeforDeleteNode(NodeUniqueId nodeUid)
{
    if (m_nodes.count(nodeUid) == 0)
    {
        SPDLOG_ERROR("handling an nonexisting node, please check it! nodeUid = {}", nodeUid);
        return;
    }

    // Node& node = m_nodes[nodeUid]; // require that Node has a default constructor
    Node& node = m_nodes.at(nodeUid); // require that Node has a default constructor
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
        if (m_outportPorts.count(edge.GetSourcePortUid()))
        {
            m_outportPorts[edge.GetSourcePortUid()]->DeletEdge(edgeUid);
        }
        else
        {
            SPDLOG_ERROR("cannot find inports in m_outportPorts inportUid = {}",
                         edge.GetSourcePortUid());
        }

        if (m_inportPorts.count(edge.GetDestinationPortUid()))
        {
            m_inportPorts[edge.GetDestinationPortUid()]->SetEdgeUid(-1);
        }
        else
        {
            SPDLOG_ERROR("cannot find inports in m_inportPorts outputid = {}",
                         edge.GetDestinationPortUid());
        }
    }
    else
    {
        SPDLOG_ERROR("cannot find edge in m_edges, edgeUid = {}, check it!", edgeUid);
    }
}

void NodeEditor::HandleDeletingEdges()
{
    // if edges are selected and users pressed the X key, erase them 
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

    // if a link is detached from a pin of node, erase it (using ctrl + leftclikc and draggingc the pin of a edge)
    EdgeUniqueId detachedEdgeUId;
    if (ImNodes::IsLinkDestroyed(&detachedEdgeUId)) 
    {
        if (m_edges.count(detachedEdgeUId) != 0)
        {
            DeleteEdgeUidFromPort(detachedEdgeUId);
            m_edges.erase(detachedEdgeUId);
        }
        else
        {
            SPDLOG_ERROR("cannot find edge in m_edges, detachedEdgeUId = {}, check it!", detachedEdgeUId);
        }
        SPDLOG_INFO("dropped link id{}", detachedEdgeUId);
    }

    // what about this situations, is it usefull ?
    // EdgeUniqueId dropedEdgeUId;
    // if (ImNodes::IsLinkDropped(&dropedEdgeUId))  
    // {
    //     if (m_edges.count(dropedEdgeUId) != 0)
    //     {
    //         DeleteEdgeUidFromPort(dropedEdgeUId);
    //         m_edges.erase(dropedEdgeUId);
    //     }
    //     else
    //     {
    //         SPDLOG_ERROR("cannot find edge in m_edges, dropedEdgeUId = {}, check it!", dropedEdgeUId);
    //     }
    //     SPDLOG_INFO("dropped link id{}", dropedEdgeUId);
    // }
}

void NodeEditor::NodeEditorShow()
{
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

void NodeEditor::SaveState()
{
    ImNodes::SaveCurrentEditorStateToIniFile("resource/savedstates.ini");

}

void NodeEditor::NodeEditorDestroy() 
{
    SaveState();

    // test for toposort
    TopologicalSort(m_nodes, m_edges);
}
} // namespace SimpleNodeEditor
