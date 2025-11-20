#include "NodeEditor.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "imnodes_internal.h"
#include "spdlog/spdlog.h"
#include <cstdint>
#include <unordered_set>
#include <set>
#include <algorithm>
#include "imgui_stdlib.h"
#include <numeric>
// Windows headers should come after STL to avoid macro conflicts
#define NOMINMAX  // Prevent Windows from defining min/max macros
#include <Windows.h>
#include <commdlg.h>
#include <fstream>

namespace SimpleNodeEditor
{
void DebugDrawRect(ImRect rect)
{
    ImDrawList* draw_list    = ImGui::GetWindowDrawList();
    ImU32       border_color = IM_COL32(255, 0, 0, 255);

    float rounding  = 0.0f; // No corner rounding
    float thickness = 1.0f; // Border thickness
    draw_list->AddRect(rect.Min, rect.Max, border_color, rounding, 0, thickness);
}

// TODO : map with (nodetype,nodedescription) maybe more reasonable
static std::unordered_map<std::string, NodeDescription>  s_nodeDescriptionsNameDesMap;
static std::unordered_map<YamlNodeType, NodeDescription> s_nodeDescriptionsTypeDesMap;

NodeEditor::NodeEditor()
    : m_nodes(),
      m_edges(),
      m_inportPorts(),
      m_outportPorts(),
      m_nodeUidGenerator("nodeUidAllocator"),
      m_portUidGenerator("portUidAllocator"),
      m_edgeUidGenerator("edgeUidAllocator"),
      m_yamlNodeUidGenerator("yamlNodeUidAllocator"),
      m_minimap_location(ImNodesMiniMapLocation_BottomRight),
      m_needTopoSort(false),
      m_allPruningRules(),
      m_currentPruninngRule(),
      m_nodesPruned(),
      m_edgesPruned(),
      m_currentPipeLineName(),
      m_nodeStyle(ImNodes::GetStyle()),
      m_pipeLineParser()
{
    // TODO: file path may be a constant value or configed in Config.yaml?
    NodeDescriptionParser        nodeTemplateParser("./resource/NodeDescriptions.yaml");
    std::vector<NodeDescription> nodeDescriptions = nodeTemplateParser.ParseNodeDescriptions();
    for (auto& nodeD : nodeDescriptions)
    {
        // TODO : remove redundant info
        s_nodeDescriptionsTypeDesMap.emplace(nodeD.m_yamlNodeType, nodeD);
        s_nodeDescriptionsNameDesMap.emplace(nodeD.m_nodeName, nodeD);
    }
    // ImNodes::GetStyle().Flags |= ImNodesStyleFlags_GridSnapping;
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

    m_portUidGenerator.Clear();
    m_nodeUidGenerator.Clear();
    m_edgeUidGenerator.Clear();
    m_pipeLineParser.Clear();
    m_pipelineEimtter.Clear();
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

    if (m_pipeLineParser.LoadFile(filePath))
    {
        std::vector<YamlNode> yamlNodes = m_pipeLineParser.ParseNodes();
        std::vector<YamlEdge> yamlEdges = m_pipeLineParser.ParseEdges();

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
                       yamlEdge,
                       false /*avoidMultipleInputLinks*/); // allow multiple edges there, multiple
 // inportEdges will be pruned later
        }

        CollectPruningRules(yamlNodes, yamlEdges);
        if (ApplyPruningRule(m_currentPruninngRule, m_nodes, m_edges))
        {
            for (const auto& [group, type] : m_currentPruninngRule)
            {
                SPDLOG_INFO(
                    "currentpruning rule is : group[{}] type[{}], any node or edge that matches "
                    "the "
                    "group but not matches the type will be removed",
                    group, type);
            }
        }
        else
        {
            SPDLOG_ERROR("ApplyPruningRule Fail!!");
        }

        m_needTopoSort        = true;
        m_currentPipeLineName = m_pipeLineParser.GetPipelineName();
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
            SPDLOG_ERROR("NodeUid[{}], InPortUid[{}], still has Edge ", node.GetNodeUniqueId(),
                         port.GetPortUniqueId());
            return false;
        }
    }

    for (OutputPort& port : node.GetOutputPorts())
    {
        if (!port.HasNoEdgeLinked())
        {
            SPDLOG_ERROR("NodeUid[{}], OutPortUid[{}], still has Edge ", node.GetNodeUniqueId(),
                         port.GetPortUniqueId());
            return false;
        }
    }

    return true;
}

bool NodeEditor::IsAllEdgesWillBePruned(NodeUniqueId                            nodeUid,
                                        const std::unordered_set<EdgeUniqueId>& shouldBeDeleteEdges)
{
    const Node&                      node  = m_nodes.at(nodeUid);
    const std::vector<EdgeUniqueId>& edges = node.GetAllEdgeUids();

    bool ret = true;
    for (EdgeUniqueId edgeUid : edges)
    {
        if (!shouldBeDeleteEdges.contains(edgeUid))
        {
            SPDLOG_ERROR("NodeUid[{}] NodeYamlId[{}], still has EdgeUid[{}] not pruned", nodeUid,
                         m_nodes.at(nodeUid).GetYamlNode().m_nodeYamlId, edgeUid);
            ret = false;
        }
    }
    return ret;
}

bool NodeEditor::ApplyPruningRule(
    const std::unordered_map<std::string, std::string>& currentPruningRule,
    std::unordered_map<NodeUniqueId, Node>              nodesMap,
    std::unordered_map<EdgeUniqueId, Edge>              edgesMap)
{
    bool                             applyPruningRuleSuccess = true;
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
                            SPDLOG_ERROR(
                                "Not All edges of Node({}) has been pruned, please check if all "
                                "edges has the correspoding prune rule!!!",
                                nodeUid);
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
            DeleteEdge(edgeUid, false /*shouldUnregisterUid*/);
        }

        for (NodeUniqueId nodeUid : shouldBeDeleteNodes)
        {
            m_nodesPruned.insert(*m_nodes.find(nodeUid));
            DeleteNode(nodeUid, false /*shouldUnregisterUid*/);
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


void NodeEditor::NodeEditorInitialize()
{
    ImNodesIO& io                           = ImNodes::GetIO();
    io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
}

void NodeEditor::MenuStyle()
{
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
}

void NodeEditor::MenuFile()
{
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Open", nullptr, false, true))
        {
            SPDLOG_INFO("Open Not Implemented!");
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Save", "Ctrl + d", false, true))
        {
            SPDLOG_INFO("Save Not Implemented!");
        }
        if (ImGui::MenuItem("Save As...", nullptr, false, true))
        {
            SPDLOG_INFO("Save as Not Implemented!");
        }

        ImGui::EndMenu();
    }
}

void NodeEditor::DrawMenu()
{
    if (ImGui::BeginMenuBar())
    {
        MenuFile();
        // ShowMiniMapMenu();
        MenuStyle();
        ImGui::EndMenuBar();
    }
}

NodeUniqueId NodeEditor::AddNewNodes(const NodeDescription& nodeDesc)
{
    YamlNode newYamlNode;
    newYamlNode.m_nodeYamlId   = m_yamlNodeUidGenerator.AllocUniqueID();
    newYamlNode.m_nodeYamlType = nodeDesc.m_yamlNodeType;
    newYamlNode.m_isSrcNode    = 0;
    newYamlNode.m_nodeName     = nodeDesc.m_nodeName;

    return AddNewNodes(nodeDesc, newYamlNode);
}

// TODO : redundant infomation here
NodeUniqueId NodeEditor::AddNewNodes(const NodeDescription& nodeDesc, const YamlNode& yamlNode)
{
    assert(yamlNode.m_nodeYamlId != -1);
    m_yamlNodeUidGenerator.RegisterUniqueID(yamlNode.m_nodeYamlId);

    Node newNode(m_nodeUidGenerator.AllocUniqueID(), Node::NodeType::NormalNode, yamlNode, nodeDesc,
                 m_nodeStyle);

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

std::optional<std::pair<std::string_view, ImVec2>> ComboFilterCombination(const std::string& popupName, const std::vector<std::string_view>& selectList)
{
    std::optional<std::pair<std::string_view, ImVec2>> ret = std::nullopt;

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows |
                               ImGuiFocusedFlags_NoPopupHierarchy) &&
        ImNodes::IsEditorHovered() && ImGui::IsKeyReleased(ImGuiKey_A)) // Imnode related
    {
        ImGui::OpenPopup(popupName.c_str());
    }

    static std::string_view previewSelected = *selectList.begin();

    if (ImGui::BeginPopup(popupName.c_str()))
    {
        const ImVec2 click_pos = ImNodes::GetCurrentContext()->NodeEditorImgCtx->IO.MousePos; // Imnode related

        ImGuiComboFlags flags       = 0;
        bool            is_selected = false;

        if (ImGui::BeginCombo("##addnode", previewSelected.data(), flags))
        {
            static ImGuiTextFilter filter;
            if (ImGui::IsWindowAppearing())
            {
                ImGui::SetKeyboardFocusHere();
                filter.Clear();
            }
            filter.Draw("##Filter", -FLT_MIN);

            for (auto& selectName : selectList)
            {
                if (filter.PassFilter(selectName.data()))
                {
                    ImGui::Selectable(selectName.data());
                    if (ImGui::IsItemActive() && ImGui::IsItemClicked())
                    {
                        SPDLOG_INFO("User has selected {}, will be added in canvas", selectName);
                        previewSelected = selectName;
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
            ret = std::optional(std::make_pair(previewSelected, click_pos));
        }
        ImGui::EndPopup();
    }
    return ret;
}

void NodeEditor::HandleAddNodes()
{
    const auto& keyViews{std::views::keys(s_nodeDescriptionsNameDesMap)};
    auto ret = ComboFilterCombination("add node", {keyViews.begin(), keyViews.end()});
    if (ret)
    {
        NodeDescription selectedNodeDescription =
            s_nodeDescriptionsNameDesMap.at(ret.value().first.data());

        NodeUniqueId newNodeUId = AddNewNodes(selectedNodeDescription);

        ImNodes::SetNodeScreenSpacePos(newNodeUId, ret.value().second);
    }
}

void NodeEditor::ShowNodes()
{
    for (auto& [nodeUid, node] : m_nodes)
    {
        ImNodes::BeginNode(nodeUid);
        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted(node.GetNodeTitle().data());
        ImNodes::EndNodeTitleBar();

        // We want input and output labels to form two aligned columns even when the counts
        // differ. Compute the max label widths and render rows = max(in_count, out_count).
        const auto&  inPorts  = node.GetInputPorts();
        const auto&  outPorts = node.GetOutputPorts();
        const size_t inCount  = inPorts.size();
        const size_t outCount = outPorts.size();
        const size_t rows     = std::max(inCount, outCount);

        // compute max widths
        float maxInLabelWidth  = 0.0f;
        float maxOutLabelWidth = 0.0f;
        for (const auto& p : inPorts)
        {
            auto n = p.GetPortname();
            maxInLabelWidth =
                std::max(maxInLabelWidth, ImGui::CalcTextSize(n.data(), n.data() + n.size()).x);
        }
        for (const auto& p : outPorts)
        {
            auto n = p.GetPortname();
            maxOutLabelWidth =
                std::max(maxOutLabelWidth, ImGui::CalcTextSize(n.data(), n.data() + n.size()).x);
        }

        // small gap between columns
        const float innerGap = 8.0f;

        // remember the starting X of the content area so we can compute column X positions
        const float baseX      = ImGui::GetCursorPosX();
        const float outColumnX = baseX + maxInLabelWidth + innerGap;

        for (size_t row = 0; row < rows; ++row)
        {
            // Input column (left)
            if (row < inCount)
            {
                const InputPort& ip = inPorts[row];
                ImNodes::BeginInputAttribute(ip.GetPortUniqueId());
                ImGui::TextUnformatted(ip.GetPortname().data());
                ImNodes::EndInputAttribute();
            }
            else
            {
                // reserve the same vertical space as a normal label so rows remain aligned
                ImGui::Dummy(ImVec2(maxInLabelWidth, ImGui::GetTextLineHeight()));
            }

            // move to output column
            ImGui::SameLine();
            ImGui::SetCursorPosX(outColumnX);

            if (row < outCount)
            {
                const OutputPort& op = outPorts[row];
                // Measure text width and right-align it within the output column
                auto        name  = op.GetPortname();
                const float textW = ImGui::CalcTextSize(name.data(), name.data() + name.size()).x;
                const float textPosX = outColumnX + (maxOutLabelWidth - textW);

                ImNodes::BeginOutputAttribute(op.GetPortUniqueId());
                // Position the text so its right edge aligns with the column's right edge
                ImGui::SetCursorPosX(textPosX);
                ImGui::TextUnformatted(op.GetPortname().data());
                ImNodes::EndOutputAttribute();
            }
            else
            {
                // reserve space for missing output
                ImGui::Dummy(ImVec2(maxOutLabelWidth, ImGui::GetTextLineHeight()));
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

std::optional<YamlPruningRule*> GetMatchedPruningRuleByGroup(
    const YamlPruningRule& findRule, std::vector<YamlPruningRule>& ruleContainer)
{
    auto iter = std::find_if(ruleContainer.begin(), ruleContainer.end(),
                             [&findRule](const YamlPruningRule& comp)
                             { return findRule.m_Group == comp.m_Group; });

    if (iter != ruleContainer.end())
    {
        return &*iter;
    }
    return std::nullopt;
}

void NodeEditor::SyncPruningRuleBetweenNodeAndEdge(const Node& node, Edge& edge)
{
    // any pruning rule that the node has but the edge does not have will be automatically added in
    // the yamledge if the pruning rule of node and edge are confilc, should we complain an error?
    // shutdown the APP?
    for (const auto& pruningRule : node.GetYamlNode().m_PruningRules)
    {
        auto ret = GetMatchedPruningRuleByGroup(pruningRule,
                                                edge.GetYamlEdge().m_yamlDstPort.m_PruningRules);
        if (ret)
        {
            if (ret.value()->m_Type != pruningRule.m_Type)
            {
                SPDLOG_WARN(
                    "Pruning rules confliced, edge's rule will be overried! nodeUid[{}] nodeName[{}] "
                    "pruning_mGroup[{}] pruning_mType[{}];"
                    "edgeUid[{}] edgeSrcPortName[{}] edgeDstPortName[{}] pruning_mGroup[{}] "
                    "pruning_mType[{}]",
                    node.GetNodeUniqueId(), node.GetNodeTitle(), pruningRule.m_Group,
                    pruningRule.m_Type, edge.GetEdgeUniqueId(),
                    edge.GetYamlEdge().m_yamlSrcPort.m_portName,
                    edge.GetYamlEdge().m_yamlDstPort.m_portName, ret.value()->m_Group,
                    ret.value()->m_Type);
                ret.value()->m_Type = pruningRule.m_Type;
            }
        }
        else
        {
            edge.GetYamlEdge().m_yamlDstPort.m_PruningRules.push_back(pruningRule);
        }
    }
}

void NodeEditor::FillYamlEdgePort(YamlPort& yamlPort, const Port& port)
{
    const Node& node      = m_nodes.at(port.OwnedByNodeUid());
    yamlPort.m_nodeName   = node.GetNodeTitle();
    yamlPort.m_nodeYamlId = node.GetYamlNode().m_nodeYamlId;
    yamlPort.m_portName   = port.GetPortname();
    yamlPort.m_portYamlId = port.GetPortYamlId();
}

void NodeEditor::AddNewEdge(PortUniqueId srcPortUid, PortUniqueId dstPortUid,
                            const YamlEdge& yamlEdge, bool avoidMultipleInputLinks)
{
    // avoid multiple edges linking to the same inport
    if (avoidMultipleInputLinks && IsInportAlreadyHasEdge(dstPortUid))
    {
        SPDLOG_WARN("inport port can not have multiple edges, inportUid[{}] portName[{}]",
                    dstPortUid, m_inportPorts.at(dstPortUid)->GetPortname());
        return;
    }

    Edge newEdge(srcPortUid, dstPortUid, m_edgeUidGenerator.AllocUniqueID(), yamlEdge);

    // set inportport's edgeid
    if (m_inportPorts.count(dstPortUid) != 0 && m_inportPorts[dstPortUid] != nullptr)
    {
        InputPort& inputPort = *m_inportPorts[dstPortUid];
        inputPort.SetEdgeUid(newEdge.GetEdgeUniqueId());
        newEdge.SetDestinationNodeUid(inputPort.OwnedByNodeUid());
        if (!yamlEdge.m_isValid)
        {
            FillYamlEdgePort(newEdge.GetYamlEdge().m_yamlDstPort, inputPort);
        }
        SyncPruningRuleBetweenNodeAndEdge(m_nodes.at(inputPort.OwnedByNodeUid()), newEdge);
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
        if (!yamlEdge.m_isValid)
        {
            FillYamlEdgePort(newEdge.GetYamlEdge().m_yamlSrcPort, outpurPort);
        }
        SyncPruningRuleBetweenNodeAndEdge(m_nodes.at(outpurPort.OwnedByNodeUid()), newEdge);
    }
    else
    {
        SPDLOG_ERROR(
            "not find output port or the pointer is nullptr when handling new edges, check it! "
            "srcPortUid is {}",
            srcPortUid);
    }
    DumpEdge(newEdge);
    newEdge.GetYamlEdge().m_isValid = true;
    m_edges.emplace(newEdge.GetEdgeUniqueId(), (newEdge));
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

void NodeEditor::DeleteEdgesBeforDeleteNode(NodeUniqueId nodeUid, bool shouldUnregisterUid)
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
            DeleteEdge(inPort.GetEdgeUid(), shouldUnregisterUid);
        }
    }

    for (OutputPort& outPort : node.GetOutputPorts())
    {
        for (EdgeUniqueId edgeUid : outPort.GetEdgeUids())
        {
            DeleteEdge(edgeUid, shouldUnregisterUid);
        }
    }

    // check that we has already deletes all edges from Node
    for (InputPort& inPort : node.GetInputPorts())
    {
        assert(inPort.GetEdgeUid() == -1);
    }

    for (OutputPort& outPort : node.GetOutputPorts())
    {
        assert(outPort.GetEdgeUids().size() == 0);
    }
}

void NodeEditor::DeleteNode(NodeUniqueId nodeUid, bool shouldUnregisterUid)
{
    if (m_nodes.count(nodeUid) == 0)
    {
        SPDLOG_ERROR("deleting a nonexisting node, check it! nodeUid = {}", nodeUid);
        return;
    }
    // before we erase the node, we need delete the linked edge first
    DeleteEdgesBeforDeleteNode(nodeUid, shouldUnregisterUid);

    // erase pointer in m_inportPorts and m_outportPorts
    auto erasePointersInMember =
        [this](const auto& portsBelongToNode, auto& refsToPorts, bool shouldUnregisterUid)
    {
        for (const auto& port : portsBelongToNode)
        {
            refsToPorts.erase(port.GetPortUniqueId());
            if (shouldUnregisterUid)
            {
                // also need to unregister portUids
                m_portUidGenerator.UnregisterUniqueID(port.GetPortUniqueId());
            }
        }
    };
    erasePointersInMember(m_nodes.at(nodeUid).GetInputPorts(), m_inportPorts, shouldUnregisterUid);
    erasePointersInMember(m_nodes.at(nodeUid).GetOutputPorts(), m_outportPorts,
                          shouldUnregisterUid);

    if (shouldUnregisterUid)
    {
        m_nodeUidGenerator.UnregisterUniqueID(nodeUid);
        m_yamlNodeUidGenerator.UnregisterUniqueID(m_nodes.at(nodeUid).GetYamlNode().m_nodeYamlId);
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
            DeleteNode(nodeUid, true /*shouldUnregisterUid*/);
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

void NodeEditor::DeleteEdge(EdgeUniqueId edgeUid, bool shouldUnregisterUid)
{
    if (m_edges.count(edgeUid) == 0)
    {
        SPDLOG_WARN("try to delte a non-exist edgeUid[{}]", edgeUid);
        return;
    }
    DeleteEdgeUidFromPort(edgeUid);

    if (shouldUnregisterUid)
    {
        m_edgeUidGenerator.UnregisterUniqueID(edgeUid);
    }

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
                DeleteEdge(edgeUid, true);
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
            DeleteEdge(detachedEdgeUId, true);
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
            SPDLOG_INFO("nodeuid[{}], nodewidth[{}]", nodeUid,
                        ImNodes::GetNodeRect(nodeUid).GetWidth());
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
    SPDLOG_INFO("RestorePruning Begin");

    assert(originType != newType);

    SPDLOG_INFO("Restoring prunerule, group[{}], originType[{}], newType[{}]", changedGroup,
                originType, newType);
    // Restore pruned nodes first so ports exist when re-attaching edges.
    for (auto it = m_nodesPruned.begin(); it != m_nodesPruned.end();)
    {
        const NodeUniqueId nodeUid = it->first;
        Node&              node    = it->second;
        bool               erasedOne = false;
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
                    erasedOne = true;
                }
                else
                {
                    SPDLOG_ERROR("Pruned Node existed in m_nodesMap with nodeuid[{}], check it!",
                                 nodeUid);
                }
            }
        }
        if (!erasedOne) ++it;
    }

    // Restore pruned edges and reattach them to ports.
    for (auto it = m_edgesPruned.begin(); it != m_edgesPruned.end();)
    {
        const EdgeUniqueId edgeUid = it->first;
        Edge&              edge    = it->second;
        bool               erasedOne = false;
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
                    erasedOne = true;
                    it = m_edgesPruned.erase(it);
                }
                else
                {
                    SPDLOG_ERROR("Pruned Node existed in m_edgesMap with edgeUid[{}], check it!",
                                 edgeUid);
                }
            }
        }
        if (!erasedOne) ++it;
    }

    // we need topo sort after resotre nodes and edges
    m_needTopoSort = true;
    SPDLOG_INFO("RestorePruning done");
}

bool NodeEditor::AddNewPruningRule(const std::string& newPruningGroup, const std::string& newPruningType,
                                   std::unordered_map<std::string, std::set<std::string>>& allPruningRule)
{
    if (newPruningGroup.empty() || newPruningType.empty())
    {
        return false;
    }

    if (allPruningRule.contains(newPruningGroup))
    {
        if (allPruningRule.at(newPruningGroup).contains(newPruningType))
        {
            SPDLOG_WARN("try to add exised pruning rule");
            return false;
        }
        else
        {
            allPruningRule.at(newPruningGroup).insert(newPruningType);
        }
    }
    else
    {
        allPruningRule.emplace(newPruningGroup, std::set{newPruningType});
    }

    return true;
}
void NodeEditor::ShowPruningRuleEditWinddow(const ImVec2& mainWindowDisplaySize)
{
    ImVec2 pruningRuleEditorWindowSize{mainWindowDisplaySize.x / 4, mainWindowDisplaySize.y / 4};
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
    // ImGuiCond_Appearing must be set, otherwise the window can not be moved or resized
    ImGui::SetNextWindowPos(ImVec2(30.f, 30.f), ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(pruningRuleEditorWindowSize, ImGuiCond_Appearing);
    ImGui::Begin("PruningRuleEdit", nullptr, ImGuiWindowFlags_None);
    static bool editing = false;
    static bool firstShowInputText = false;
    ImGui::TextUnformatted("Pruning Rules"); ImGui::SameLine();
    if (ImGui::SmallButton("New"))
    {
        editing = true;
        firstShowInputText = true;
    }
    ImGui::Separator();

    for (auto& groupPair : m_allPruningRules)
    {
        const std::string            group    = groupPair.first;
        const std::set<std::string>& typesSet = groupPair.second;

        std::vector<std::string> types(typesSet.begin(), typesSet.end());
        std::string treeRootNodeName{group};
        if (ImGui::TreeNodeEx(treeRootNodeName.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Bullet))
        {
            for (size_t iterIndex = 0; iterIndex < types.size(); ++iterIndex)
            {
                bool isSelected = iterIndex == GetMatchedIndex(types, m_currentPruninngRule.at(group));
                if (ImGui::Selectable(types[iterIndex].c_str(), isSelected))
                {
                    if (m_currentPruninngRule[group] != types[iterIndex])
                    {
                        SPDLOG_INFO("Select a different pruning rule, change the grapgh  ...");
                        std::string originType{m_currentPruninngRule[group]};
                        m_currentPruninngRule[group] = types[iterIndex];
                        // m_nodes and m_edges can not pass by reference, see the logic of
                        // ApplyPruningRule: erase elem of m_nodes and m_edges in the iteration, but
                        // has not handled the erased iterator properly
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
            ImGui::TreePop();
        }
        ImGui::Separator();
    }

    static std::string newPruneGroup{};
    static std::string newPruneType{};
    if (editing)
    {
        ImGui::PushItemWidth(50.f);
        if (firstShowInputText) { ImGui::SetKeyboardFocusHere(); firstShowInputText = false; }
        ImGui::InputText("New Group", &newPruneGroup); ImGui::SameLine();
        ImGui::InputText("New Type", &newPruneType); ImGui::SameLine();
        ImGui::SetNextItemShortcut(ImGuiKey_Enter);
        if (ImGui::Button("Done"))
        {
            if (AddNewPruningRule(newPruneGroup, newPruneType, m_allPruningRules))
            {
                if (!m_currentPruninngRule.contains(newPruneGroup))
                {
                    m_currentPruninngRule[newPruneGroup] = newPruneType;
                }
            }
            editing = false;
            newPruneGroup.clear();
            newPruneType.clear();
        }
        ImGui::PopItemWidth();
    }

    ImGui::End(); // end of pruningRuleEditorWindow
    ImGui::PopStyleVar();
}

void NodeEditor::SyncPruningRules(const Node& node)
{
    for (EdgeUniqueId edgeUid : node.GetAllEdgeUids())
    {
        SyncPruningRuleBetweenNodeAndEdge(node, m_edges.at(edgeUid));
    }
}

void NodeEditor::HandleNodeInfoEditing()
{
    static NodeUniqueId nodeUidToBePoped{-1};
    static YamlNode     popUpYamlNode{};
    NodeUniqueId        selectedNode{-1};
    // handle userinteractions to prepare for popup
    if (ImNodes::IsNodeHovered(&selectedNode) && ImGui::IsMouseDoubleClicked(0))
    {
        assert(selectedNode != -1);
        SPDLOG_INFO("node with nodeUid[{}] has been double clicked", selectedNode);

        nodeUidToBePoped = selectedNode;
        // Initialize popup buffers from the node's current YAML data
        if (m_nodes.contains(selectedNode))
        {
            const YamlNode& yn = m_nodes.at(selectedNode).GetYamlNode();
            popUpYamlNode = yn;
            ImGui::SetNextWindowPos(ImGui::GetMousePos());
            ImGui::OpenPopup("Node Info Editor");
        }
    }

    // begin popopup
    ImGuiIO& io                    = ImGui::GetIO();
    ImVec2   mainWindowDisplaySize = io.DisplaySize;
    ImGui::SetNextWindowSize(ImVec2{mainWindowDisplaySize.x / 4, mainWindowDisplaySize.y / 4});
    if (ImGui::BeginPopupModal("Node Info Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_HorizontalScrollbar))
    {
        // show basic node info
        ImGui::Text("NodeInfoEditor");
        ImGui::Separator();
        ImGui::Text("BasicInfos:");
        ImGui::Text("NodeUid: %d", nodeUidToBePoped);
        ImGui::Text("NodeName: "); ImGui::SameLine();
        ImGui::InputText("##NodeName", &popUpYamlNode.m_nodeName);
        ImGui::Text("YamlId: %d", popUpYamlNode.m_nodeYamlId);
        ImGui::Text("YamlType: %d", popUpYamlNode.m_nodeYamlType);
        ImGui::Checkbox("IsSource ", reinterpret_cast<bool*>(&popUpYamlNode.m_isSrcNode));
        ImGui::Separator();

        // node property show
        ImGui::TextUnformatted("Properties: ");
        if (ImGui::BeginTable("PropertiesTable", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Id", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < popUpYamlNode.m_Properties.size(); ++i)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                const std::string prop_name_label = std::string("##prop_name_") + std::to_string(i);
                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::InputText(prop_name_label.c_str(), &popUpYamlNode.m_Properties[i].m_propertyName, ImGuiInputTextFlags_CharsNoBlank);

                ImGui::TableSetColumnIndex(1);
                const std::string prop_id_label = std::string("##prop_id_") + std::to_string(i);
                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::InputInt(prop_id_label.c_str(), &popUpYamlNode.m_Properties[i].m_propertyId);


                ImGui::TableSetColumnIndex(2);
                const std::string prop_value_label = std::string("##prop_val_") + std::to_string(i);
                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::InputText(prop_value_label.c_str(), &popUpYamlNode.m_Properties[i].m_propertyValue);
            }
            ImGui::EndTable();
        }
        // add node property
        if (ImGui::SmallButton("AddProperty"))
        {
            ImGui::SetNextWindowPos(ImGui::GetMousePos(), ImGuiCond_Appearing);
            ImGui::OpenPopup("AddNodeProperty");
        }

        if (ImGui::BeginPopupModal("AddNodeProperty", nullptr,
                                    ImGuiWindowFlags_AlwaysAutoResize))
        {   
            ImGui::PushItemWidth(70.f);
            YamlNodeProperty newProperty;
            ImGui::InputText("propertyName", &newProperty.m_propertyName); ImGui::SameLine();
            ImGui::InputInt("propertyId", &newProperty.m_propertyId); ImGui::SameLine();
            ImGui::InputText("propertyValue", &newProperty.m_propertyValue); ImGui::SameLine();
            ImGui::PopItemWidth();

            ImGui::PushItemWidth(50.f);
            if (ImGui::Button("Done") )
            {
                popUpYamlNode.m_Properties.push_back(std::move(newProperty));
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            ImGui::SetNextItemShortcut(ImGuiKey_Escape);
            if (ImGui::Button("Cancel")) { ImGui::CloseCurrentPopup(); }
            ImGui::PopItemWidth();
            ImGui::EndPopup();
        }

        // PruningRule Show
        ImGui::Separator();
        ImGui::TextUnformatted("Pruning Rules:");
        int remove_node_prune_idx = -1;
        auto shouldBeDeleted = popUpYamlNode.m_PruningRules.end(); // static or normal variable?
        for (auto iter = popUpYamlNode.m_PruningRules.begin(); iter != popUpYamlNode.m_PruningRules.end(); ++iter)
        {
            ImGui::PushItemWidth(50.f);
            ImGui::TextUnformatted(iter->m_Group.c_str ()); ImGui::SameLine();
            ImGui::TextUnformatted(iter->m_Type.c_str()); ImGui::SameLine();
            // PruningRule Delete
            if (ImGui::SmallButton((std::string("Remove").c_str())))
            {
                shouldBeDeleted = iter;
            }
            ImGui::PopItemWidth();
        }

        if (shouldBeDeleted != popUpYamlNode.m_PruningRules.end())
        {
            popUpYamlNode.m_PruningRules.erase(shouldBeDeleted);
        }

        ImGui::Spacing();

        // add new pruning rule
        if (ImGui::SmallButton("AddPruningRule"))
        {
            ImGui::SetNextWindowPos(ImGui::GetMousePos(), ImGuiCond_Appearing);
            if (!m_allPruningRules.empty()) { ImGui::OpenPopup("AddNodePruningRule"); }
            else {SPDLOG_ERROR("Please Add global pruning rule please");}
        }

        // Modal popup: choose a group/type from m_allPruningRules 
        if (ImGui::BeginPopupModal("AddNodePruningRule", nullptr,
                                    ImGuiWindowFlags_AlwaysAutoResize))
        {
            std::vector<std::string_view> groups{(m_allPruningRules | std::views::keys).begin(),
                                (m_allPruningRules | std::views::keys).end()};
            static int selectedGroupIndex = 0;
            if (ImGui::BeginCombo("Group", groups[selectedGroupIndex].data()))
            {
                for (size_t i = 0; i < groups.size(); ++i)
                {
                    bool isSelelected = (selectedGroupIndex == i);
                    if (ImGui::Selectable(groups[i].data(), isSelelected)) selectedGroupIndex = i;
                    if (isSelelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            const auto& typeSet = m_allPruningRules.at(groups[selectedGroupIndex].data());
            std::vector<std::string_view> types{typeSet.begin(), typeSet.end()};
            static int selectedTypeIndex = 0;
            // types for selected group
            if (ImGui::BeginCombo("Type", types[selectedTypeIndex].data()))
            {
                for (int i = 0; i < (int)types.size(); ++i)
                {
                    bool is_sel = (selectedTypeIndex == i);
                    if (ImGui::Selectable(types[i].data(), is_sel)) selectedTypeIndex = i;
                    if (is_sel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::Separator();

            if (ImGui::Button("Done") )
            {
                std::string_view chosenGroup = groups.empty() ? std::string() : groups[selectedGroupIndex];
                std::string_view chosenType = types[selectedTypeIndex];

                if (!chosenGroup.empty() && !chosenType.empty())
                {
                    // avoid duplicates
                    bool addIt = true;
                    for (auto& [iterGroup, iterType] : popUpYamlNode.m_PruningRules)
                    {
                        if (iterGroup == chosenGroup)
                        {
                            SPDLOG_WARN("This rule[Group[{}],Type[{}]] belongs to the same group may override the original one !", chosenGroup, chosenType);
                            addIt = false;
                            break;
                        }
                    }
                    if (addIt) popUpYamlNode.m_PruningRules.push_back({chosenGroup.data(), chosenType.data()});
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            ImGui::SetNextItemShortcut(ImGuiKey_Escape);
            if (ImGui::Button("Cancel")) { ImGui::CloseCurrentPopup(); }

            ImGui::EndPopup();
        }

        ImGui::Separator();
        // Save node info or cancel to close popup
        {
            const float availW  = ImGui::GetContentRegionAvail().x;
            ImGuiStyle& style   = ImGui::GetStyle();
            const float spacing = style.ItemSpacing.x * 2;
            const float btnW    = (availW - spacing) * 0.5f;

            if (ImGui::Button("Save", ImVec2(btnW, 0)))
            {
                // Commit edited values back into the node's YamlNode
                m_nodes.at(nodeUidToBePoped).GetYamlNode() = popUpYamlNode;
                m_nodes.at(nodeUidToBePoped).SetNodeTitle(popUpYamlNode.m_nodeName + "_" + std::to_string(popUpYamlNode.m_nodeYamlId));
                // sync pruning rule between node and edges
                SyncPruningRules(m_nodes.at(nodeUidToBePoped));
                if (ApplyPruningRule(m_currentPruninngRule, m_nodes, m_edges))
                {
                    SPDLOG_INFO("ApplyPruningRuleSuccess");
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            ImGui::SetNextItemShortcut(ImGuiKey_Escape);
            if (ImGui::Button("CancelNodeInfo", ImVec2(btnW, 0)))
            {
                ImGui::CloseCurrentPopup();
            }

        }
        ImGui::EndPopup();
    }
}

void deleteMatchedGroup(std::vector<std::string_view>& groups, const std::vector<YamlPruningRule>& pruningRules)
{
    std::unordered_set<std::string_view> groupsToRemove;
    for (const auto& rule : pruningRules) {
        groupsToRemove.insert(rule.m_Group);
    }

    groups.erase(
        std::remove_if(groups.begin(), groups.end(),
            [&groupsToRemove](std::string_view group) {
                // 检查当前group是否需要被剔除
                return groupsToRemove.count(group) > 0;
            }),
        groups.end()
    );
}

void NodeEditor::HandleEdgeInfoEditing()
{
    static EdgeUniqueId edgeUidToBePoped{-1};
    static YamlEdge     popUpYamlEdge{};
    EdgeUniqueId        selectedEdge{-1};

    // handle user interaction to prepare for the popup
    if (ImNodes::IsLinkHovered(&selectedEdge) && ImGui::IsMouseDoubleClicked(0))
    {
        assert(selectedEdge != -1);
        SPDLOG_INFO("Edge with EdgeUid[{}] has been double clicked", selectedEdge);
        edgeUidToBePoped = selectedEdge;
        if (m_edges.contains(selectedEdge))
        {
            popUpYamlEdge = m_edges.at(selectedEdge).GetYamlEdge();
            ImGui::SetNextWindowPos(ImGui::GetMousePos());
            ImGui::OpenPopup("Edge Info Editor");
        }
    }

    ImGuiIO& io                    = ImGui::GetIO();
    ImVec2   mainWindowDisplaySize = io.DisplaySize;
    ImGui::SetNextWindowSize(ImVec2{mainWindowDisplaySize.x / 4, mainWindowDisplaySize.y / 4});

    if (ImGui::BeginPopupModal("Edge Info Editor", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGui::Text("Edge UID: %d", edgeUidToBePoped);
        ImGui::Separator();

        // Source Port (read-only)
        ImGui::TextUnformatted("Source Port:");
        ImGui::Text("NodeId: %d", popUpYamlEdge.m_yamlSrcPort.m_nodeYamlId);
        ImGui::TextUnformatted((std::string("NodeName: ") + popUpYamlEdge.m_yamlSrcPort.m_nodeName).c_str());
        ImGui::Text("PortId: %d", popUpYamlEdge.m_yamlSrcPort.m_portYamlId);
        ImGui::TextUnformatted((std::string("PortName: ") + popUpYamlEdge.m_yamlSrcPort.m_portName).c_str());

        ImGui::Separator();

        // Destination Port (read-only)
        ImGui::TextUnformatted("Destination Port:");
        ImGui::Text("NodeId: %d", popUpYamlEdge.m_yamlDstPort.m_nodeYamlId);
        ImGui::TextUnformatted((std::string("NodeName: ") + popUpYamlEdge.m_yamlDstPort.m_nodeName).c_str());
        ImGui::Text("PortId: %d", popUpYamlEdge.m_yamlDstPort.m_portYamlId);
        ImGui::TextUnformatted((std::string("PortName: ") + popUpYamlEdge.m_yamlDstPort.m_portName).c_str());

        ImGui::Separator();
        ImGui::TextUnformatted("DestinationPortPruningRules:");

        int remove_prune_idx = -1;
        auto shouldBeDeleted = popUpYamlEdge.m_yamlDstPort.m_PruningRules.end();
        Node& srcNode = m_nodes.at(m_edges.at(edgeUidToBePoped).GetSourceNodeUid());
        Node& dstNode = m_nodes.at(m_edges.at(edgeUidToBePoped).GetDestinationNodeUid());

        for (auto iter = popUpYamlEdge.m_yamlDstPort.m_PruningRules.begin(); iter != popUpYamlEdge.m_yamlDstPort.m_PruningRules.end(); ++iter)
        {
            ImGui::PushItemWidth(50.f);
            ImGui::TextUnformatted(iter->m_Group.c_str ()); ImGui::SameLine();
            ImGui::TextUnformatted(iter->m_Type.c_str());
            // PruningRule Delete
            // onlu when the attached node does not has the pruning rule can users delete the pruning rule
            if (!GetMatchedPruningRuleByGroup(*iter, srcNode.GetYamlNode().m_PruningRules) &&
                !GetMatchedPruningRuleByGroup(*iter, dstNode.GetYamlNode().m_PruningRules) ) 
            {
                ImGui::SameLine();
                if (ImGui::SmallButton((std::string("Remove").c_str())))
                {
                    shouldBeDeleted = iter;
                }
            }
            ImGui::PopItemWidth();
        }

        if (shouldBeDeleted != popUpYamlEdge.m_yamlDstPort.m_PruningRules.end())
        {
            popUpYamlEdge.m_yamlDstPort.m_PruningRules.erase(shouldBeDeleted);
        }

        ImGui::Spacing();

        if (ImGui::SmallButton("New"))
        {
            ImGui::SetNextWindowPos(ImGui::GetMousePos(), ImGuiCond_Appearing);
            if (!m_allPruningRules.empty()) { ImGui::OpenPopup("AddEdgePruningRule"); }
        }

                // Modal popup: choose a group/type from m_allPruningRules 
        if (ImGui::BeginPopupModal("AddEdgePruningRule", nullptr,
                                    ImGuiWindowFlags_AlwaysAutoResize))
        {
            std::vector<std::string_view> groups{(m_allPruningRules | std::views::keys).begin(),
                                (m_allPruningRules | std::views::keys).end()};
            static int selectedGroupIndex = 0;
            if (ImGui::BeginCombo("Group", groups[selectedGroupIndex].data()))
            {
                for (size_t i = 0; i < groups.size(); ++i)
                {
                    bool isSelelected = (selectedGroupIndex == i);
                    if (ImGui::Selectable(groups[i].data(), isSelelected)) selectedGroupIndex = i;
                    if (isSelelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            const auto& typeSet = m_allPruningRules.at(groups[selectedGroupIndex].data());
            std::vector<std::string_view> types{typeSet.begin(), typeSet.end()};
            static int selectedTypeIndex = 0;
            // types for selected group
            if (ImGui::BeginCombo("Type", types[selectedTypeIndex].data()))
            {
                for (int i = 0; i < (int)types.size(); ++i)
                {
                    bool is_sel = (selectedTypeIndex == i);
                    if (ImGui::Selectable(types[i].data(), is_sel)) selectedTypeIndex = i;
                    if (is_sel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::Separator();
            if (ImGui::Button("Done") )
            {
                std::string_view chosenGroup = groups.empty() ? std::string() : groups[selectedGroupIndex];
                std::string_view chosenType = types[selectedTypeIndex];

                if (!chosenGroup.empty() && !chosenType.empty())
                {
                    // avoid duplicates
                    bool addIt = true;
                    for (auto& [iterGroup, iterType] : popUpYamlEdge.m_yamlDstPort.m_PruningRules)
                    {
                        if (iterGroup == chosenGroup)
                        {
                            SPDLOG_WARN("This rule[Group[{}],Type[{}]] belongs to the same group may override the original one !", chosenGroup, chosenType);
                            addIt = false;
                            break;
                        }
                    }
                    if (addIt) popUpYamlEdge.m_yamlDstPort.m_PruningRules.push_back({chosenGroup.data(), chosenType.data()});
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            ImGui::SetNextItemShortcut(ImGuiKey_Escape);
            if (ImGui::Button("Cancel")) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }

        ImGui::Separator();
        // Make Save/Cancel buttons share the available width evenly in Edge popup
        {
            const float availW  = ImGui::GetContentRegionAvail().x;
            ImGuiStyle& style   = ImGui::GetStyle();
            const float spacing = style.ItemSpacing.x;
            const float btnW    = (availW - spacing) * 0.4f;

            if (ImGui::Button("Save", ImVec2(btnW, 0)))
            {
                // Commit edited values back into the edge's YamlEdge
                if (m_edges.contains(edgeUidToBePoped))
                {
                    m_edges.at(edgeUidToBePoped).GetYamlEdge() = popUpYamlEdge;
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            ImGui::SetNextItemShortcut(ImGuiKey_Escape);
            if (ImGui::Button("Cancel", ImVec2(btnW, 0))) { ImGui::CloseCurrentPopup(); }
        }

        ImGui::EndPopup();
        
    }
}

void NodeEditor::SaveToFile()
{
    const std::string pipelineNameToEmit =
        m_currentPipeLineName.empty() ? m_pipeLineParser.GetPipelineName() : m_currentPipeLineName;
    m_pipelineEimtter.EmitPipeline(pipelineNameToEmit, m_nodes, m_nodesPruned, m_edges,
                                   m_edgesPruned);
    SPDLOG_INFO(m_pipelineEimtter.GetEmitter().c_str());

    // Show file save dialog
    char          szFile[MAX_PATH] = {0};
    OPENFILENAMEA ofn              = {0};
    ofn.lStructSize                = sizeof(OPENFILENAMEA);
    ofn.hwndOwner                  = nullptr;
    ofn.lpstrFile                  = szFile;
    ofn.nMaxFile                   = sizeof(szFile);
    ofn.lpstrFilter                = "YAML Files (*.yaml)\0*.yaml\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex               = 1;
    ofn.lpstrTitle                 = "Save Pipeline As";
    ofn.Flags                      = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt                = "yaml";

    // Prefill the save dialog with a sensible default filename based on the pipeline name
    std::string defaultFileName = pipelineNameToEmit + ".yaml";
    strncpy(szFile, defaultFileName.c_str(), sizeof(szFile) - 1);
    szFile[sizeof(szFile) - 1] = '\0';

    if (GetSaveFileNameA(&ofn))
    {
        try
        {
            std::ofstream outFile(szFile);
            if (outFile.is_open())
            {
                outFile << m_pipelineEimtter.GetEmitter().c_str();
                outFile.close();
                SPDLOG_INFO("Successfully saved pipeline to: {}", szFile);
            }
            else
            {
                SPDLOG_ERROR("Failed to open file for writing: {}", szFile);
            }
        }
        catch (const std::exception& e)
        {
            SPDLOG_ERROR("Error while saving file: {}", e.what());
        }
    }
    else
    {
        // Only log error if user didn't just cancel
        DWORD error = CommDlgExtendedError();
        if (error != 0)
        {
            SPDLOG_ERROR("File save dialog error: {}", error);
        }
    }
}

void NodeEditor::ShowPipelineName()
{
    ImGuiStyle& style = ImGui::GetStyle();

    const ImVec2 contentMin   = ImGui::GetWindowContentRegionMin();
    const ImVec2 contentMax   = ImGui::GetWindowContentRegionMax();
    const float  contentWidth = contentMax.x - contentMin.x;

    const float textH   = ImGui::GetTextLineHeight();
    const float footerH = textH + style.FramePadding.y * 2.0f;

    ImGui::SetCursorPosY(contentMax.y - footerH);
    auto windFlags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;
    ImGui::BeginChild("PipelineFooter", ImVec2(contentWidth, footerH), 0, windFlags);
    // show pipelinename
    ImGui::PushItemWidth(260.0f);
    ImGui::TextUnformatted("PipelineName:"); ImGui::SameLine();
    ImGui::TextColored(COLOR_RED, m_currentPipeLineName.c_str()); ImGui::SameLine();
    ImGui::PopItemWidth();
    if (ImGui::SmallButton("EditName"))
    {
        ImGui::OpenPopup("PipelineNameChange");
        ImVec2   mainWindowDisplaySize = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowSize(ImVec2{mainWindowDisplaySize.x / 4, mainWindowDisplaySize.y / 4});
    }

    static std::string newPipeLineName {m_currentPipeLineName};
    if (ImGui::BeginPopupModal("PipelineNameChange", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("PiepelineName: "); ImGui::SameLine();
        if (ImGui::IsWindowAppearing())
        {
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::InputText("##pipelineNameInput", &newPipeLineName); ImGui::SameLine();
        ImGui::SetNextItemShortcut(ImGuiKey_Enter);
        if (ImGui::SmallButton("Done"))
        {
            m_currentPipeLineName = newPipeLineName;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::EndChild();
}

void NodeEditor::HandleOtherUserInputs()
{
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_NoPopupHierarchy |
                               ImGuiFocusedFlags_RootAndChildWindows) &&
        ImNodes::IsEditorHovered())
    {
        ImGuiIO& io = ImGui::GetIO();
        // ctrl + S : handle saving to file, i.e serialize the pipeline to a yaml
        if (ImGui::IsKeyPressed(ImGuiKey_S) && io.KeyCtrl)
        {
            SaveToFile();
        }

        // S : toposort
        if (ImGui::IsKeyReleased(ImGuiKey_S) && !io.KeyCtrl)
        {
            m_needTopoSort = true;
        }
    }


}

void NodeEditor::HandleZooming()
{
    if (ImNodes::IsEditorHovered() && ImGui::GetIO().MouseWheel != 0 &&
        !ImGui::IsPopupOpen("add node", ImGuiPopupFlags_AnyPopup))
    {
        float zoom = ImNodes::EditorContextGetZoom() + ImGui::GetIO().MouseWheel * 0.1f;
        ImNodes::EditorContextSetZoom(zoom, ImGui::GetMousePos());
    }
}

void NodeEditor::ShowGrapghEditWindow(const ImVec2& mainWindowDisplaySize)
{
    auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                 ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(mainWindowDisplaySize);

    // The node editor window
    ImGui::Begin("SimpleNodeEditor", nullptr, flags);
    ImNodes::GetIO().EmulateThreeButtonMouse.Modifier = &ImGui::GetIO().KeyAlt;

    DrawMenu();

    ImNodes::BeginNodeEditor();
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

    ShowPipelineName();

    HandleZooming();
    HandleAddNodes();
    HandleAddEdges();

    HandleDeletingEdges();

    HandleDeletingNodes();

    HandleNodeInfoEditing();
    HandleEdgeInfoEditing();

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
