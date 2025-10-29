#include "NodeEditor.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "spdlog/spdlog.h"
#include <cstdint>
#include "Helpers.h"
#include <unordered_set>
#include "YamlParser.hpp"
#include <set>

namespace SimpleNodeEditor
{

// TODO : map with (nodetype,nodedescription) maybe more reasonable
static std::unordered_map<std::string, NodeDescription>  s_nodeDescriptionsNameDesMap;
static std::unordered_map<YamlNodeType, NodeDescription> s_nodeDescriptionsTypeDesMap;

NodeEditor::NodeEditor()
    : m_nodes(),
      m_edges(),
      m_inportPorts(),
      m_outportPorts(),
      m_nodeUidGenerator(),
      m_portUidGenerator(),
      m_edgeUidGenerator(),
      m_minimap_location(ImNodesMiniMapLocation_BottomRight),
      m_needTopoSort(false),
      m_allPruningRules()
{
    // TODO: file path may be a constant value or configed in Config.yaml?
    NodeDescriptionParser        nodeParser("./resource/NodeDescriptions.yaml");
    std::vector<NodeDescription> nodeDescriptions = nodeParser.ParseNodeDescriptions();
    for (auto& nodeD : nodeDescriptions)
    {
        // TODO : remove redundant info
        s_nodeDescriptionsTypeDesMap.emplace(nodeD.m_yamlNodeType, nodeD);
        s_nodeDescriptionsNameDesMap.emplace(nodeD.m_nodeName, nodeD);
    }
}

void NodeEditor::ClearCurrentPipeLine()
{
    m_nodes.clear();
    m_edges.clear();
    m_inportPorts.clear();
    m_outportPorts.clear();
    m_allPruningRules.clear();
    m_currentPruninngRule.clear();
    m_nodesPruned.clear();
    m_edgesPruned.clear();
}

void NodeEditor::HandleFileDrop(const std::string& filePath)
{
    ClearCurrentPipeLine();
    if (LoadPipelineFromFile(filePath))
    {
        SPDLOG_INFO("LoadPipeLineFromFile Success, filePath[{}]", filePath);
    }
    else
    {
        SPDLOG_ERROR("LoadPipeLineFromFile failed, filePath[{}]", filePath);
    }
}

bool NodeEditor::LoadPipelineFromFile(const std::string& filePath)
{
    bool ret = true;

    PipelineParser pipeLineParser;
    if (pipeLineParser.LoadFile(filePath))
    {
        std::vector<YamlNode> yamlNodes = pipeLineParser.ParseNodes();
        std::vector<YamlEdge> yamlEdges = pipeLineParser.ParseEdges();


        // add node in Editor
        float                                                  x_axis = 0.f, y_axis = 0.f;
        std::unordered_map<YamlNode::NodeYamlId, NodeUniqueId> t_yamlNodeId2NodeUidMap;
        for (const YamlNode& yamlNode : yamlNodes)
        {
            NodeUniqueId newNodeUid =
                AddNewNodes(s_nodeDescriptionsTypeDesMap.at(yamlNode.m_nodeYamlType), yamlNode);
            t_yamlNodeId2NodeUidMap.emplace(yamlNode.m_nodeYamlId, newNodeUid);
        }

        // add edges in editor
        for (const YamlEdge& yamlEdge : yamlEdges)
        {
            NodeUniqueId ownedBySrcNodeUid =
                t_yamlNodeId2NodeUidMap.at(yamlEdge.m_yamlSrcPort.m_nodeYamlId);
            NodeUniqueId ownedByDstNodeUid =
                t_yamlNodeId2NodeUidMap.at(yamlEdge.m_yamlDstPort.m_nodeYamlId);
            const Node& srcNode = m_nodes.at(ownedBySrcNodeUid);
            const Node& dstNode = m_nodes.at(ownedByDstNodeUid);
            AddNewEdge(srcNode.FindPortUidAmongOutports(yamlEdge.m_yamlSrcPort.m_portYamlId),
                       dstNode.FindPortUidAmongInports(yamlEdge.m_yamlDstPort.m_portYamlId),
                       yamlEdge);
        }

        CollectPruningRules(yamlNodes, yamlEdges);
        if (ApplyPruningRule(m_currentPruninngRule, m_nodes, m_edges))
        {
            for (const auto& [group, type] : m_currentPruninngRule)
            {
                SPDLOG_INFO(
                    "currentpruning rule is : group[{}] type[{}], any node or edge that matches the "
                    "group but not matches the type will be removed",
                    group, type);
            }
        }
        else
        {
            SPDLOG_ERROR("ApplyPruningRule Fail!!");
        }

        m_needTopoSort = true;
    }
    else
    {
        SPDLOG_ERROR("pipelineparser loadfile failed, filename is [{}], check it!", filePath);
        ret = false;
    }
    return ret;
}

bool NodeEditor::IsAllEdgesHasBeenPruned(NodeUniqueId nodeUid)
{
    Node& node = m_nodes.at(nodeUid);

    for (InputPort& port : node.GetInputPorts())
    {
        if (!port.HasNoEdgeLinked())
        {
            SPDLOG_ERROR("NodeUid[{}], InPortUid[{}], still has Edge ", node.GetNodeUniqueId(), port.GetPortUniqueId());
            return false;
        }
    }

    for (OutputPort& port : node.GetOutputPorts())
    {
        if (!port.HasNoEdgeLinked())
        {
            SPDLOG_ERROR("NodeUid[{}], OutPortUid[{}], still has Edge ", node.GetNodeUniqueId(), port.GetPortUniqueId());
            return false;
        }
    }
    
    return true;
}

bool NodeEditor::IsAllEdgesWillBePruned(NodeUniqueId nodeUid, const std::unordered_set<EdgeUniqueId>& shouldBeDeleteEdges)
{
    const Node& node = m_nodes.at(nodeUid);
    const std::vector<EdgeUniqueId>& edges = node.GetAllEdges();

    bool ret = true;
    for (EdgeUniqueId edgeUid : edges)
    {
        if (!shouldBeDeleteEdges.contains(edgeUid))
        {
            SPDLOG_ERROR("NodeUid[{}] NodeYamlId[{}], still has EdgeUid[{}] not pruned", 
                        nodeUid, 
                        m_nodes.at(nodeUid).GetYamlNode().m_nodeYamlId, 
                        edgeUid);
            ret = false;
        }
    }
    return ret;
}

bool NodeEditor::ApplyPruningRule(const std::unordered_map<std::string, std::string>& currentPruningRule,
                                  std::unordered_map<NodeUniqueId, Node>       nodesMap,
                                  std::unordered_map<EdgeUniqueId, Edge>       edgesMap)
{
    bool applyPruningRuleSuccess = true;
    std::unordered_set<NodeUniqueId> shouldBeDeleteEdges;
    std::unordered_set<NodeUniqueId> shouldBeDeleteNodes;

    for (const auto& [group, type] : currentPruningRule)
    {
        // prune edges first
        for (const auto& [edgeUid, edge] : edgesMap)
        {
            for (auto& edgePruningRule : edge.GetYamlEdge().m_yamlDstPort.m_PruningRules)
            {
                if (edgePruningRule.m_Group == group && edgePruningRule.m_Type != type)
                {
                    auto iter = m_edges.find(edgeUid);
                    if (iter != m_edges.end())
                    {
                        SPDLOG_INFO(
                            "Prune Edge with EdgeUid[{}] SrcNodeUid[{}] SrcNodeYamlId[{}] "
                            "DstNodeUid[{}] DstNodeYamlId[{}]",
                            edgeUid, edge.GetSourceNodeUid(),
                            edge.GetYamlEdge().m_yamlSrcPort.m_nodeYamlId,
                            edge.GetDestinationNodeUid(),
                            edge.GetYamlEdge().m_yamlDstPort.m_nodeYamlId);
                        shouldBeDeleteEdges.insert(edgeUid);
                        // m_edgesPruned.insert(*iter);
                        // DeleteEdge(edgeUid);
                    }
                    else
                    {
                        SPDLOG_ERROR("Cannot find edge in m_edges with edgeuid[{}]", edgeUid);
                        applyPruningRuleSuccess = false;
                        break;
                    }
                }
            }
        }

        for (const auto& [nodeUid, node] : nodesMap)
        {
            for (auto& nodePruningRule : node.GetYamlNode().m_PruningRules)
            {
                if (nodePruningRule.m_Group == group && nodePruningRule.m_Type != type)
                {
                    // prune the node
                    auto iter = m_nodes.find(nodeUid);
                    if (iter != m_nodes.end())
                    {
                        if (IsAllEdgesWillBePruned(nodeUid, shouldBeDeleteEdges))
                        {
                            SPDLOG_INFO("Prune Node with NodeUid[{}] NodeYamlId[{}]", nodeUid,
                                        node.GetYamlNode().m_nodeYamlId);
                            // m_nodesPruned.insert(*iter);
                            // DeleteNode(nodeUid);
                            shouldBeDeleteNodes.insert(nodeUid);
                        }
                        else 
                        {
                            SPDLOG_ERROR("Not All edges of Node({}) has been pruned, please check if all edges has the correspoding prune rule!!!", nodeUid);
                            applyPruningRuleSuccess = false;
                            break;
                        }
                    }
                    else
                    {
                        SPDLOG_ERROR("Cannot find node in m_nodes with nodeuid[{}]", nodeUid);
                        applyPruningRuleSuccess = false;
                        break;
                    }
                }
            }
        }
    }
    
    // actually do the pruning operation
    if (applyPruningRuleSuccess)
    {
        for (EdgeUniqueId edgeUid : shouldBeDeleteEdges)
        {
            m_edgesPruned.insert(*m_edges.find(edgeUid));
            // edges will also be deleted in DeleteNode() function though
            DeleteEdge(edgeUid);
        }

        for (NodeUniqueId nodeUid : shouldBeDeleteNodes)
        {
            m_nodesPruned.insert(*m_nodes.find(nodeUid));
            DeleteNode(nodeUid);
        }
    }

    return applyPruningRuleSuccess;
}

void NodeEditor::CollectPruningRules(std::vector<YamlNode> yamlNodes,
                                     std::vector<YamlEdge> yamlEdges)
{
    // collect pruning rules from nodes and edges
    for (const YamlNode& yamlNode : yamlNodes)
    {
        for (const YamlPruningRule& pruningRule : yamlNode.m_PruningRules)
        {
            m_allPruningRules[pruningRule.m_Group].insert(pruningRule.m_Type);
        }
    }

    for (const YamlEdge& yamlEdge : yamlEdges)
    {
        for (const YamlPruningRule& pruningRule : yamlEdge.m_yamlDstPort.m_PruningRules)
        {
            m_allPruningRules[pruningRule.m_Group].insert(pruningRule.m_Type);
        }
    }

    if (m_allPruningRules.size() == 0)
    {
        SPDLOG_INFO("no pruning rule in the current yamlfile"); // todo : add yamlfilename
    }
    // set defatul pruning rule
    for (const auto& [group, type] : m_allPruningRules)
    {
        if (type.size() != 0)
        {
            if (type.size() == 1)
            {
                SPDLOG_WARN("pruning rule, group[{}] only has one type!", group);
            }
            m_currentPruninngRule[group] = *(type.begin());
            SPDLOG_INFO("set default pruning rule, group[{}] type[{}]", group, *(type.begin()));
        }
    }
}

void DebugDrawRect(ImRect rect)
{
    ImDrawList* draw_list    = ImGui::GetWindowDrawList();
    ImU32       border_color = IM_COL32(255, 0, 0, 255);

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

// TODO : redundant infomation here
NodeUniqueId NodeEditor::AddNewNodes(const NodeDescription& nodeDesc, const YamlNode& yamlNode)
{
    Node newNode(m_nodeUidGenerator.AllocUniqueID(), Node::NodeType::NormalNode, yamlNode,
                 yamlNode.m_nodeYamlId == -1
                     ? nodeDesc.m_nodeName
                     : nodeDesc.m_nodeName + std::to_string(yamlNode.m_nodeYamlId));

    NodeUniqueId ret = newNode.GetNodeUniqueId();

    // we must reserve the vector first; if not , the reallocation of std::vector will
    // mess memory up
    newNode.GetInputPorts().reserve(nodeDesc.m_inputPortNames.size());
    newNode.GetOutputPorts().reserve(nodeDesc.m_outputPortNames.size());

    // add inputports in the new node and add pointers in m_inportPorts
    for (int index = 0; index < nodeDesc.m_inputPortNames.size(); ++index)
    {
        InputPort newInport(m_portUidGenerator.AllocUniqueID(), index,
                            nodeDesc.m_inputPortNames[index], newNode.GetNodeUniqueId(), index);
        newNode.AddInputPort(newInport);
        m_inportPorts.emplace(newInport.GetPortUniqueId(),
                              newNode.GetInputPort(newInport.GetPortUniqueId()));
    }

    // add outputports in the new node and add pointers in m_outportPorts
    for (int index = 0; index < nodeDesc.m_outputPortNames.size(); ++index)
    {
        OutputPort newOutport(m_portUidGenerator.AllocUniqueID(), index,
                              nodeDesc.m_outputPortNames[index], newNode.GetNodeUniqueId(), index);
        newNode.AddOutputPort(newOutport);
        m_outportPorts.emplace(newOutport.GetPortUniqueId(),
                               newNode.GetOutputPort(newOutport.GetPortUniqueId()));
    }

    if (!m_nodes.insert({newNode.GetNodeUniqueId(), std::move(newNode)}).second)
    {
        SPDLOG_ERROR("m_nodes insert new node fail! check it!");
        return -1;
    }
    return ret;
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

    static std::string previewSelected = s_nodeDescriptionsNameDesMap.begin()->first;

    if (ImGui::BeginPopup("add node"))
    {
        const ImVec2 click_pos = ImGui::GetMousePosOnOpeningCurrentPopup();

        ImGuiComboFlags flags       = 0;
        bool            is_selected = false;

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

            for (auto& [nodeName, nodeDesc] : s_nodeDescriptionsNameDesMap)
            {
                if (filter.PassFilter(nodeName.c_str()))
                {
                    ImGui::Selectable(nodeName.c_str());
                    if (ImGui::IsItemActive() && ImGui::IsItemClicked())
                    {
                        SPDLOG_INFO("User has selected {}, will be added in canvas", nodeName);
                        previewSelected = nodeName;
                        is_selected     = true;
                        ImGui::CloseCurrentPopup(); // close Combo right now
                    }
                }
            }
            ImGui::EndCombo();
        }

        if (is_selected)
        {
            ImGui::CloseCurrentPopup(); // close popup right now

            NodeDescription selectedNodeDescription =
                s_nodeDescriptionsNameDesMap.at(previewSelected);

            NodeUniqueId newNodeUId = AddNewNodes(selectedNodeDescription);

            ImNodes::SetNodeScreenSpacePos(newNodeUId, click_pos);

            // Add logs to understand the 3 types of coordinates

            // imnode internal datastructure : ImNodeData::Origin , store the coordinates in grid
            // space. The value of this coodinate may be negative or posive, and changed with editor
            // panning events.
            ImVec2 newNodeGridSpace = ImNodes::GetNodeGridSpacePos(newNodeUId);

            // defined by app window. the value of this coordinate is definately positive.
            ImVec2 newNodeScreenSpace = ImNodes::GetNodeScreenSpacePos(newNodeUId);

            // just like a viewport coordination? the value of this coordinate is definetely
            // positive .
            ImVec2 newNodeEditorSpace = ImNodes::GetNodeEditorSpacePos(newNodeUId);

            SPDLOG_INFO(
                "add new node, nodeuid = {}, girdspace = [{},{}], screenspace = [{},{}], "
                "editorspace = [{}, {}]",
                newNodeUId, newNodeGridSpace.x, newNodeGridSpace.y, newNodeScreenSpace.x,
                newNodeScreenSpace.y, newNodeEditorSpace.x, newNodeEditorSpace.y);
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

void NodeEditor::AddNewEdge(PortUniqueId srcPortUid, PortUniqueId dstPortUid,
                            const YamlEdge& yamlEdge, bool avoidMultipleInputLinks)
{
    // avoid multiple edges linking to the same inport
    if (avoidMultipleInputLinks && IsInportAlreadyHasEdge(dstPortUid))
    {
        SPDLOG_WARN("inport port can not have multiple edges, inportUid = {}", dstPortUid);
        return;
    }

    Edge newEdge(srcPortUid, dstPortUid, m_edgeUidGenerator.AllocUniqueID(), yamlEdge);

    // set inportport's edgeid
    if (m_inportPorts.count(dstPortUid) != 0 && m_inportPorts[dstPortUid] != nullptr)
    {
        InputPort& inputPort = *m_inportPorts[dstPortUid];
        inputPort.SetEdgeUid(newEdge.GetEdgeUniqueId());
        newEdge.SetDestinationNodeUid(inputPort.OwnedByNodeUid());
    }
    else
    {
        SPDLOG_ERROR(
            "not find input port or the pointer is nullptr when handling new edges, check it! "
            "dstPortUid is{}",
            dstPortUid);
    }

    // set outportport's edgeid
    if (m_outportPorts.count(srcPortUid) != 0 && m_outportPorts[srcPortUid] != nullptr)
    {
        OutputPort& outpurPort = *m_outportPorts[srcPortUid];
        outpurPort.PushEdge(newEdge.GetEdgeUniqueId());
        newEdge.SetSourceNodeUid(outpurPort.OwnedByNodeUid());
    }
    else
    {
        SPDLOG_ERROR(
            "not find output port or the pointer is nullptr when handling new edges, check it! "
            "srcPortUid is {}",
            srcPortUid);
    }
    m_edges.emplace(newEdge.GetEdgeUniqueId(), std::move(newEdge));
}

void NodeEditor::HandleAddEdges()
{
    PortUniqueId startPortId, endPortId;

    // note : edge is linked from startPortId to endPortId
    if (ImNodes::IsLinkCreated(&startPortId, &endPortId))
    {
        AddNewEdge(startPortId, endPortId);
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

    Node& node = m_nodes.at(nodeUid);
    for (InputPort& inPort : node.GetInputPorts())
    {
        if (inPort.GetEdgeUid() != -1)
        {
            DeleteEdge(inPort.GetEdgeUid());
        }
    }

    for (OutputPort& outPort : node.GetOutputPorts())
    {
        for (EdgeUniqueId edgeUid : outPort.GetEdgeUids())
        {
            DeleteEdge(edgeUid);
        }
    }

    // runtime check that we has already delete all edges from Node
    for (InputPort& inPort : node.GetInputPorts())
    {
        assert(inPort.GetEdgeUid() == -1);
    }

    for (OutputPort& outPort : node.GetOutputPorts())
    {
        assert(outPort.GetEdgeUids().size() == 0);
    }
}

void NodeEditor::DeleteNode(NodeUniqueId nodeUid)
{
    if (m_nodes.count(nodeUid) == 0)
    {
        SPDLOG_ERROR("deleting a nonexisting node, check it! nodeUid = {}", nodeUid);
        return;
    }
    // before we erase the node, we need delete the linked edge first
    DeleteEdgesBeforDeleteNode(nodeUid);

    // erase pointer in m_inportPorts and m_outportPorts
    for (const auto& inputPort : m_nodes.at(nodeUid).GetInputPorts())
    {
        m_inportPorts.erase(inputPort.GetPortUniqueId());
    }

    for (const auto& outputPort : m_nodes.at(nodeUid).GetOutputPorts())
    {
        m_outportPorts.erase(outputPort.GetPortUniqueId());
    }

    m_nodes.erase(nodeUid);
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
            DeleteNode(nodeUid);
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

void NodeEditor::DeleteEdge(EdgeUniqueId edgeUid)
{
    if (m_edges.count(edgeUid) == 0)
    {
        SPDLOG_WARN("try to delte a non-exist edgeUid[{}]", edgeUid);
        return;
    }
    DeleteEdgeUidFromPort(edgeUid);
    m_edges.erase(edgeUid);
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
                DeleteEdge(edgeUid);
            }
            else
            {
                SPDLOG_ERROR("cannot find edge in m_edges, edgeUid = {}, check it!", edgeUid);
            }
        }
    }

    // if a link is detached from a pin of node, erase it (using ctrl + leftclikc and draggingc the
    // pin of a edge)
    EdgeUniqueId detachedEdgeUId;
    if (ImNodes::IsLinkDestroyed(&detachedEdgeUId))
    {
        if (m_edges.count(detachedEdgeUId) != 0)
        {
            DeleteEdge(detachedEdgeUId);
        }
        else
        {
            SPDLOG_ERROR("cannot find edge in m_edges, detachedEdgeUId = {}, check it!",
                         detachedEdgeUId);
        }
        SPDLOG_INFO("dropped link id{}", detachedEdgeUId);
    }

    // what about this situations, is it usefull ?
    // EdgeUniqueId dropedEdgeUId;
    // if (ImNodes::IsLinkDropped(&dropedEdgeUId))
    // {
    //     if (m_edges.count(dropedEdgeUId) != 0)
    //     {
    //          DeleteEdge(dropedEdgeUId);
    //     }
    //     else
    //     {
    //         SPDLOG_ERROR("cannot find edge in m_edges, dropedEdgeUId = {}, check it!",
    //         dropedEdgeUId);
    //     }
    //     SPDLOG_INFO("dropped link id{}", dropedEdgeUId);
    // }
}

void NodeEditor::RearrangeNodesLayout(
    const std::vector<std::vector<NodeUniqueId>>& topologicalOrder,
    const std::unordered_map<NodeUniqueId, Node>& nodesMap)
{
    if (topologicalOrder.size() == 0 || nodesMap.size() == 0)
    {
        return;
    }

    float verticalPadding   = 20.f;
    float horizontalPadding = 100.f;

    // put collections of nodes belonging to the same topological priority in a single colum
    std::vector<float> columWidths;
    std::vector<float> columHeights;
    // calc each colum's width and height using rect of NodeUi
    for (const auto& nodeUIdVec : topologicalOrder)
    {
        float columWidth  = 0.f; // equal to the max of nodes' width in the same priority
        float columHeight = 0.f;
        for (NodeUniqueId nodeUid : nodeUIdVec)
        {
            columWidth = std::max(ImNodes::GetNodeRect(nodeUid).GetWidth(), columWidth);
            columHeight += (ImNodes::GetNodeRect(nodeUid).GetHeight() + verticalPadding);
        }
        columWidths.push_back(columWidth);
        columHeights.push_back(columHeight);
    }

    assert(topologicalOrder.size() == columWidths.size() &&
           topologicalOrder.size() == columHeights.size());

    float gridSpaceX = 0.f;
    for (size_t columIndex = 0; columIndex < topologicalOrder.size(); ++columIndex)
    {
        float gridSpaceY = -columHeights[columIndex] / 2.0f;
        for (const NodeUniqueId nodeUid : topologicalOrder[columIndex])
        {
            ImNodes::SetNodeGridSpacePos(nodeUid, ImVec2{gridSpaceX, gridSpaceY});
            gridSpaceY += (ImNodes::GetNodeRect(nodeUid).GetHeight() + verticalPadding);
        }
        gridSpaceX += (columWidths[columIndex] + horizontalPadding);
    }

    // set editor panning
    ImNodes::EditorContextResetPanning(ImVec2{0, ImGui::GetWindowHeight() / 2.0f});
}

// resotre nodes and edges that match the currentPruningRule
void NodeEditor::RestorePruning(const std::string& changedGroup, const std::string& originType,
                                const std::string& newType)
{
    SPDLOG_ERROR("RestorePruning Begin");

    assert(originType != newType);

    SPDLOG_INFO("Restoring prunerule, group[{}], originType[{}], newType[{}]", changedGroup,
                originType, newType);
    // Restore pruned nodes first so ports exist when re-attaching edges.
    for (auto it = m_nodesPruned.begin(); it != m_nodesPruned.end();)
    {
        const NodeUniqueId nodeUid = it->first;
        Node&              node    = it->second;

        for (const auto& pruningRule : node.GetYamlNode().m_PruningRules)
        {
            if (pruningRule.m_Group == changedGroup && pruningRule.m_Type == newType)
            {
                assert(pruningRule.m_Type != originType);
                if (m_nodes.find(nodeUid) == m_nodes.end())
                {
                    SPDLOG_INFO("Resotre node with nodeUid[{}], yamlNodeName[{}], yamlNodeId[{}]",
                                nodeUid, node.GetYamlNode().m_nodeName,
                                node.GetYamlNode().m_nodeYamlId);

                    m_nodes.emplace(nodeUid, std::move(node));

                    // Re-populate port lookup maps for the restored node.
                    Node& restoredNode = m_nodes.at(nodeUid);
                    for (InputPort& inPort : restoredNode.GetInputPorts())
                    {
                        m_inportPorts[inPort.GetPortUniqueId()] =
                            restoredNode.GetInputPort(inPort.GetPortUniqueId());
                    }
                    for (OutputPort& outPort : restoredNode.GetOutputPorts())
                    {
                        m_outportPorts[outPort.GetPortUniqueId()] =
                            restoredNode.GetOutputPort(outPort.GetPortUniqueId());
                    }

                    it = m_nodesPruned.erase(it);
                }
                else
                {
                    SPDLOG_ERROR("Pruned Node existed in m_nodesMap with nodeuid[{}], check it!",
                                 nodeUid);
                    ++it;
                }
            }
            else
            {
                ++it;
            }
        }
    }

    // Restore pruned edges and reattach them to ports.
    for (auto it = m_edgesPruned.begin(); it != m_edgesPruned.end();)
    {
        const EdgeUniqueId edgeUid = it->first;
        Edge&              edge    = it->second;

        for (const auto& pruningRule : edge.GetYamlEdge().m_yamlDstPort.m_PruningRules)
        {
            if (pruningRule.m_Group == changedGroup && pruningRule.m_Type == newType)
            {
                assert(pruningRule.m_Type != originType);
                if (m_edges.find(edgeUid) == m_edges.end())
                {
                    SPDLOG_INFO(
                        "restore edge with edgeUid[{}], yamlSrcPortName[{}], yamlDstPortName[{}]",
                        edgeUid, edge.GetYamlEdge().m_yamlSrcPort.m_portName,
                        edge.GetYamlEdge().m_yamlDstPort.m_portName);
                    m_edges.emplace(edgeUid, std::move(edge));
                    Edge& restoredEdge = m_edges.at(edgeUid);
                    // Reattach to input port
                    PortUniqueId dstPort = restoredEdge.GetDestinationPortUid();
                    m_inportPorts.at(dstPort)->SetEdgeUid(edgeUid);

                    // Reattach to output port
                    PortUniqueId srcPort = restoredEdge.GetSourcePortUid();
                    m_outportPorts.at(srcPort)->PushEdge(edgeUid);

                    it = m_edgesPruned.erase(it);
                }
                else
                {
                    SPDLOG_ERROR("Pruned edge existed in m_edgesMap with edgeuid[{}], check it!",
                                 edgeUid);
                    ++it;
                }
            }
            else
            {
                ++it;
            }
        }
    }

    // we need topo sort after resotre nodes and edges
    m_needTopoSort = true;
    SPDLOG_ERROR("RestorePruning done");
}

void NodeEditor::ShowPruningRuleEditWinddow(const ImVec2& mainWindowDisplaySize)
{
    ImVec2 pruningRuleEditorWindowSize{mainWindowDisplaySize.x / 5, mainWindowDisplaySize.y / 5};
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
    // ImGuiCond_Appearing must be set, otherwise the window can not be moved or resized
    ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(pruningRuleEditorWindowSize, ImGuiCond_Appearing);
    ImGui::Begin("PruningRuleEdit", nullptr, ImGuiWindowFlags_None);

    // Pruning rules UI: show each group and a combo to pick the active type.
    ImGui::TextUnformatted("Pruning Rules");
    ImGui::Separator();

    for (auto& groupPair : m_allPruningRules)
    {
        const std::string            group    = groupPair.first;
        const std::set<std::string>& typesSet = groupPair.second;

        ImGui::TextUnformatted(group.c_str());
        ImGui::SameLine();
        // copy set into vector to allow indexed access in the combo
        std::vector<std::string> types(typesSet.begin(), typesSet.end());

        std::string& current = m_currentPruninngRule.at(group);
        const char*  preview = current.empty() ? "(none)" : current.c_str();

        ImGui::PushID(group.c_str());
        if (ImGui::BeginCombo("##prune_combo", preview))
        {
            for (size_t i = 0; i < types.size(); ++i)
            {
                if (ImGui::Selectable(types[i].c_str()))
                {
                    if (m_currentPruninngRule[group] != types[i])
                    {
                        SPDLOG_INFO("Select a different pruning rule, change the grapgh  ...");
                        std::string originType{m_currentPruninngRule[group]};
                        m_currentPruninngRule[group] = types[i];

                        // m_nodes and m_edges can not pass by reference, see the logic of ApplyPruningRule: erase elem of m_nodes and m_edges in the iteration, but has not handled the erased iterator properly
                        // TODO : change this to pass by reference
                        if (ApplyPruningRule(m_currentPruninngRule, m_nodes, m_edges))
                        {
                            RestorePruning(group, originType, m_currentPruninngRule[group]);
                        }
                        else 
                        {
                            m_currentPruninngRule[group] = originType;
                        }


                    }
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopID();
        ImGui::Separator();
    }

    ImGui::End(); // end of pruningRuleEditorWindow
    ImGui::PopStyleVar();
}

void NodeEditor::HandleOtherUserInputs()
{
    if (ImGui::IsKeyReleased(ImGuiKey_S))
    {
        m_needTopoSort = true;
    }

    NodeUniqueId selectedNode = -1;
    EdgeUniqueId selectedEdge = -1;

    // Local static state for the popup so we don't need class-wide members or header changes.
    static bool     popup_active = false;
    static NodeUniqueId popup_node_id = -1;
    static char     popup_name_buf[256] = {};
    static int      popup_yaml_id = -1;
    static bool     popup_is_src = false;
    static int      popup_yaml_type = 0;
    // support up to 32 properties in the editor popup; increase if you need more
    static const int MAX_PROPS = 32;
    static char     popup_prop_bufs[MAX_PROPS][256];
    static int      popup_prop_count = 0;

    // Edge popup state (function-local to avoid header changes)
    static bool        edge_popup_active = false;
    static EdgeUniqueId edge_popup_id = -1;
    static char        edge_src_node_name[128] = {};
    static int         edge_src_node_id = -1;
    static char        edge_src_port_name[128] = {};
    static int         edge_src_port_id = -1;
    static char        edge_dst_node_name[128] = {};
    static int         edge_dst_node_id = -1;
    static char        edge_dst_port_name[128] = {};
    static int         edge_dst_port_id = -1;
    // pruning rules for dst port (editable in the edge popup)
    static const int   EDGE_MAX_PRUNE_RULES = 8;
    static char        edge_dst_prune_group[EDGE_MAX_PRUNE_RULES][64] = {};
    static char        edge_dst_prune_type[EDGE_MAX_PRUNE_RULES][64] = {};
    static int         edge_dst_prune_count = 0;

    if (ImNodes::IsNodeHovered(&selectedNode) && ImGui::IsMouseDoubleClicked(0))
    {
        assert(selectedNode != -1);
        SPDLOG_INFO("node with nodeUid[{}] has been double clicked", selectedNode);

        // Initialize popup buffers from the node's current YAML data
        if (m_nodes.find(selectedNode) != m_nodes.end())
        {
            const YamlNode& yn = m_nodes.at(selectedNode).GetYamlNode();
            strncpy(popup_name_buf, yn.m_nodeName.c_str(), sizeof(popup_name_buf) - 1);
            popup_yaml_id = static_cast<int>(yn.m_nodeYamlId);
            popup_is_src = yn.m_isSrcNode;
            popup_yaml_type = static_cast<int>(yn.m_nodeYamlType);

            popup_prop_count = (int)std::min<size_t>(yn.m_Properties.size(), MAX_PROPS);
            for (int i = 0; i < popup_prop_count; ++i)
            {
                strncpy(popup_prop_bufs[i], yn.m_Properties[i].m_propertyValue.c_str(), sizeof(popup_prop_bufs[i]) - 1);
            }

            popup_node_id = selectedNode;
            popup_active = true;
            ImGui::OpenPopup("Node Info Editor");
        }
    }

    // Render the popup modal every frame (BeginPopupModal handles visibility)
    if (ImGui::BeginPopupModal("Node Info Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (!popup_active || popup_node_id == -1 || m_nodes.find(popup_node_id) == m_nodes.end())
        {
            ImGui::TextUnformatted("No node selected or node was removed.");
            if (ImGui::Button("Close"))
            {
                popup_active = false;
                popup_node_id = -1;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        else
        {
            ImGui::Text("Node UID: %d", popup_node_id);
            ImGui::Separator();

            // Editable YAML-backed fields
            ImGui::InputText("Name", popup_name_buf, sizeof(popup_name_buf));
            ImGui::InputInt("Yaml Id", &popup_yaml_id);
            ImGui::Checkbox("Is Source", &popup_is_src);
            ImGui::InputInt("Yaml Type", &popup_yaml_type);

            ImGui::Separator();
            ImGui::TextUnformatted("Properties:");
            for (int i = 0; i < popup_prop_count; ++i)
            {
                // show property name (read-only) then editable value
                const std::string prop_label = std::string("##prop_val_") + std::to_string(i);
                ImGui::TextUnformatted(m_nodes.at(popup_node_id).GetYamlNode().m_Properties[i].m_propertyName.c_str());
                ImGui::SameLine();
                ImGui::InputText(prop_label.c_str(), popup_prop_bufs[i], sizeof(popup_prop_bufs[i]));
            }

            ImGui::Separator();
            if (ImGui::Button("Save"))
            {
                // Commit edited values back into the node's YamlNode
                YamlNode& yn = m_nodes.at(popup_node_id).GetYamlNode();
                yn.m_nodeName = std::string(popup_name_buf);
                yn.m_nodeYamlId = static_cast<YamlNode::NodeYamlId>(popup_yaml_id);
                yn.m_isSrcNode = popup_is_src;
                yn.m_nodeYamlType = static_cast<YamlNodeType>(popup_yaml_type);
                for (int i = 0; i < popup_prop_count; ++i)
                {
                    yn.m_Properties[i].m_propertyValue = std::string(popup_prop_bufs[i]);
                }

                popup_active = false;
                popup_node_id = -1;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                // Discard changes (we only wrote into buffers)
                popup_active = false;
                popup_node_id = -1;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    if (ImNodes::IsLinkHovered(&selectedEdge) && ImGui::IsMouseDoubleClicked(0))
    {
        assert(selectedEdge != -1);
        SPDLOG_INFO("Edge with EdgeUid[{}] has been double clicked", selectedEdge);

        // Initialize edge popup buffers from the edge's current YAML data
        if (m_edges.find(selectedEdge) != m_edges.end())
        {
            YamlEdge& ye = m_edges.at(selectedEdge).GetYamlEdge();
            // src
            strncpy(edge_src_node_name, ye.m_yamlSrcPort.m_nodeName.c_str(), sizeof(edge_src_node_name) - 1);
            edge_src_node_id = static_cast<int>(ye.m_yamlSrcPort.m_nodeYamlId);
            strncpy(edge_src_port_name, ye.m_yamlSrcPort.m_portName.c_str(), sizeof(edge_src_port_name) - 1);
            edge_src_port_id = static_cast<int>(ye.m_yamlSrcPort.m_portYamlId);
            // dst
            strncpy(edge_dst_node_name, ye.m_yamlDstPort.m_nodeName.c_str(), sizeof(edge_dst_node_name) - 1);
            edge_dst_node_id = static_cast<int>(ye.m_yamlDstPort.m_nodeYamlId);
            strncpy(edge_dst_port_name, ye.m_yamlDstPort.m_portName.c_str(), sizeof(edge_dst_port_name) - 1);
            edge_dst_port_id = static_cast<int>(ye.m_yamlDstPort.m_portYamlId);
            // dst pruning rules init
            edge_dst_prune_count = (int)std::min<size_t>(ye.m_yamlDstPort.m_PruningRules.size(), (size_t)EDGE_MAX_PRUNE_RULES);
            for (int pi = 0; pi < edge_dst_prune_count; ++pi)
            {
                const YamlPruningRule& pr = ye.m_yamlDstPort.m_PruningRules[pi];
                strncpy(edge_dst_prune_group[pi], pr.m_Group.c_str(), sizeof(edge_dst_prune_group[pi]) - 1);
                strncpy(edge_dst_prune_type[pi], pr.m_Type.c_str(), sizeof(edge_dst_prune_type[pi]) - 1);
            }

            edge_popup_id = selectedEdge;
            edge_popup_active = true;
            ImGui::OpenPopup("Edge Info Editor");
        }
    }

    // Edge editor popup modal
    if (ImGui::BeginPopupModal("Edge Info Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (!edge_popup_active || edge_popup_id == -1 || m_edges.find(edge_popup_id) == m_edges.end())
        {
            ImGui::TextUnformatted("No edge selected or edge was removed.");
            if (ImGui::Button("Close"))
            {
                edge_popup_active = false;
                edge_popup_id = -1;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        else
        {
            ImGui::Text("Edge UID: %d", edge_popup_id);
            ImGui::Separator();

            ImGui::TextUnformatted("Source Port:");
            ImGui::InputText("Src Node Name", edge_src_node_name, sizeof(edge_src_node_name));
            ImGui::InputInt("Src Node Yaml Id", &edge_src_node_id);
            ImGui::InputText("Src Port Name", edge_src_port_name, sizeof(edge_src_port_name));
            ImGui::InputInt("Src Port Yaml Id", &edge_src_port_id);

            ImGui::Separator();
            ImGui::TextUnformatted("Destination Port:");
            ImGui::InputText("Dst Node Name", edge_dst_node_name, sizeof(edge_dst_node_name));
            ImGui::InputInt("Dst Node Yaml Id", &edge_dst_node_id);
            ImGui::InputText("Dst Port Name", edge_dst_port_name, sizeof(edge_dst_port_name));
            ImGui::InputInt("Dst Port Yaml Id", &edge_dst_port_id);

            ImGui::Separator();
            ImGui::TextUnformatted("Destination Port Pruning Rules:");
            // allow removing one rule per-frame (deferred index)
            int remove_prune_idx = -1;
            for (int i = 0; i < edge_dst_prune_count; ++i)
            {
                // unique labels
                std::string grp_label = std::string("Group ##dst_prune_grp_") + std::to_string(i);
                std::string typ_label = std::string("Type  ##dst_prune_typ_") + std::to_string(i);
                ImGui::InputText(grp_label.c_str(), edge_dst_prune_group[i], sizeof(edge_dst_prune_group[i]));
                ImGui::SameLine();
                ImGui::InputText(typ_label.c_str(), edge_dst_prune_type[i], sizeof(edge_dst_prune_type[i]));
                ImGui::SameLine();
                if (ImGui::SmallButton((std::string("Remove") + std::to_string(i)).c_str()))
                {
                    remove_prune_idx = i;
                }
            }
            if (remove_prune_idx != -1)
            {
                for (int j = remove_prune_idx; j + 1 < edge_dst_prune_count; ++j)
                {
                    strncpy(edge_dst_prune_group[j], edge_dst_prune_group[j + 1], sizeof(edge_dst_prune_group[j]) - 1);
                    strncpy(edge_dst_prune_type[j], edge_dst_prune_type[j + 1], sizeof(edge_dst_prune_type[j]) - 1);
                }
                // clear last
                edge_dst_prune_group[edge_dst_prune_count - 1][0] = '\0';
                edge_dst_prune_type[edge_dst_prune_count - 1][0] = '\0';
                edge_dst_prune_count -= 1;
            }
            ImGui::Spacing();
            if (edge_dst_prune_count < EDGE_MAX_PRUNE_RULES)
            {
                if (ImGui::Button("Add Prune Rule"))
                {
                    edge_dst_prune_group[edge_dst_prune_count][0] = '\0';
                    edge_dst_prune_type[edge_dst_prune_count][0] = '\0';
                    edge_dst_prune_count += 1;
                }
            }

            ImGui::Separator();
            if (ImGui::Button("Save"))
            {
                // Commit edited values back into the edge's YamlEdge
                YamlEdge& ye = m_edges.at(edge_popup_id).GetYamlEdge();
                ye.m_yamlSrcPort.m_nodeName = std::string(edge_src_node_name);
                ye.m_yamlSrcPort.m_nodeYamlId = static_cast<YamlNode::NodeYamlId>(edge_src_node_id);
                ye.m_yamlSrcPort.m_portName = std::string(edge_src_port_name);
                ye.m_yamlSrcPort.m_portYamlId = static_cast<YamlPort::PortYamlId>(edge_src_port_id);

                ye.m_yamlDstPort.m_nodeName = std::string(edge_dst_node_name);
                ye.m_yamlDstPort.m_nodeYamlId = static_cast<YamlNode::NodeYamlId>(edge_dst_node_id);
                ye.m_yamlDstPort.m_portName = std::string(edge_dst_port_name);
                ye.m_yamlDstPort.m_portYamlId = static_cast<YamlPort::PortYamlId>(edge_dst_port_id);

                // commit dst pruning rules
                ye.m_yamlDstPort.m_PruningRules.clear();
                for (int i = 0; i < edge_dst_prune_count; ++i)
                {
                    YamlPruningRule pr;
                    pr.m_Group = std::string(edge_dst_prune_group[i]);
                    pr.m_Type = std::string(edge_dst_prune_type[i]);
                    ye.m_yamlDstPort.m_PruningRules.push_back(std::move(pr));
                }

                edge_popup_active = false;
                edge_popup_id = -1;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                edge_popup_active = false;
                edge_popup_id = -1;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}

void NodeEditor::ShowGrapghEditWindow(const ImVec2& mainWindowDisplaySize)
{
    auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                 ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(mainWindowDisplaySize);

    // The node editor window
    ImGui::Begin("SimpleNodeEditor", nullptr, flags);
    ImNodes::GetIO().EmulateThreeButtonMouse.Modifier = &ImGui::GetIO().KeyAlt;
    ImNodes::BeginNodeEditor();

    HandleAddNodes();

    ShowNodes();
    // only afer calling ImNodes::EndNode in the ShowNodes() function can we get the rect of nodeUi,
    // and then we do toposort

    if (m_needTopoSort)
    {
        const auto& topoSortRes = TopologicalSort(m_nodes, m_edges);
        RearrangeNodesLayout(topoSortRes, m_nodes);
        m_needTopoSort = false;
    }

    ShowEdges();

    ImNodes::MiniMap(0.2f, m_minimap_location);

    ImNodes::EndNodeEditor();

    HandleAddEdges();

    HandleDeletingEdges();

    HandleDeletingNodes();

    // S : toposort; double click(left mouse button) event
    HandleOtherUserInputs();
    ImGui::End(); // end of "SimpleNodeEditor"
}

void NodeEditor::NodeEditorShow()
{
    ImGuiIO& io                    = ImGui::GetIO();
    ImVec2   mainWindowDisplaySize = io.DisplaySize;

    ShowGrapghEditWindow(mainWindowDisplaySize);
    ShowPruningRuleEditWinddow(mainWindowDisplaySize);
}

void NodeEditor::SaveState()
{
    ImNodes::SaveCurrentEditorStateToIniFile("resource/savedstates.ini");
}

void NodeEditor::NodeEditorDestroy()
{
    SaveState();
}
} // namespace SimpleNodeEditor
