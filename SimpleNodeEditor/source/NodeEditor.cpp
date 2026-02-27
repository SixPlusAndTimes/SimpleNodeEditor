#include "NodeEditor.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "imnodes_internal.h"
#include "imgui_stdlib.h"
#include "Log.hpp"
#include "Common.hpp"
#include "Notify.hpp"
#include "FileDialog.hpp"
#include <cstdint>
#include <unordered_set>
#include <set>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <optional>

namespace SimpleNodeEditor
{

// Helper function for GetMatchedPruningRuleByGroup - used in HandleEdgeInfoEditing
static std::optional<YamlPruningRule*> GetMatchedPruningRuleByGroup(
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
void DebugDrawRect(ImRect rect)
{
    ImDrawList* draw_list    = ImGui::GetWindowDrawList();
    ImU32       border_color = IM_COL32(255, 0, 0, 255);
    float       rounding     = 0.0f; // No corner rounding
    float       thickness    = 1.0f; // Border thickness
    draw_list->AddRect(rect.Min, rect.Max, border_color, rounding, 0, thickness);
}

void DegbugDrawCircle(const ImVec2& center, float radius)
{
    ImU32       color     = IM_COL32(255, 0, 0, 255);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddCircleFilled(center, radius, color);
}

// User preference for port visibility mode

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
      m_minimap_location(ImNodesMiniMapLocation_TopRight),
      m_needTopoSort(false),
      m_currentPipeLineName(),
      m_nodeStyle(&ImNodes::GetStyle()),
      m_pipeLineParser(),
      m_fileDialog(),
      m_commandQueue(),
      m_pruningPolicy(),
      m_hideUnlinkedPorts(false)
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

}

void NodeEditor::NodeEditorInitialize()
{
    ImNodesIO& io                           = ImNodes::GetIO();
    io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
}

void NodeEditor::DrawFileDialog()
{
    if (m_fileDialog.Draw())
    {
        if (m_fileDialog.GetType() == FileDialog::Type::OPEN)
        {
            ClearCurrentPipeLine(); // should let user confirm to do this ? 
            LoadPipeline(m_fileDialog.GetResultInStream());
        }else if (m_fileDialog.GetType() == FileDialog::Type::SAVE)
        {
            SNELOG_INFO("save pipeline file to {}", m_fileDialog.GetFileName().String());
            SaveToFile(m_fileDialog.GetResultOutStream());
        }
    }
}
void NodeEditor::NodeEditorShow()
{
    ImGuiIO& io                    = ImGui::GetIO();
    ImVec2   mainWindowDisplaySize = io.DisplaySize;

    ShowGrapghEditWindow(mainWindowDisplaySize);
    ShowPruningRuleEditWinddow(mainWindowDisplaySize);
    Notifier::Draw();
    DrawFileDialog();
}

void NodeEditor::NodeEditorDestroy() {}


void MenuStyle()
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
        ImGui::EndMenu();
    }
}

void NodeEditor::DrawFileMenu()
{
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Open", "Ctrl + o", false, true))
        {
            m_fileDialog.MarkFileDialogOpen();
        }
        if (ImGui::MenuItem("Save", "Ctrl + s", false, true))
        {
            m_fileDialog.MarkFileDialogSave("untitled.yaml");
        }
        if (ImGui::MenuItem("Clear", "Ctrl + l", false, true))
        {
            ClearCurrentPipeLine();
        }

        ImGui::EndMenu();
    }
}

void NodeEditor::DrawConfig()
{
    if (ImGui::BeginMenu("Config"))
    {
        ImGui::Checkbox("Hide Unlinked Ports", &m_hideUnlinkedPorts);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Toggle between showing all ports and hiding unlinked ports");
        }
        ImGui::EndMenu();
    }
}

void NodeEditor::DrawMenu()
{
    if (ImGui::BeginMenuBar())
    {
        DrawFileMenu();
        // ShowMiniMapMenu();
        MenuStyle();
        DrawConfig();
        ImGui::EndMenuBar();
    }
}

void NodeEditor::ShowNodes()
{
    for (auto& [nodeUid, node] : m_nodes)
    {
        OpacitySetter opacitySetter(node.GetOpacity(),
                                    ImNodesCol_NodeBackground,
                                    ImNodesCol_NodeBackgroundHovered,
                                    ImNodesCol_NodeBackgroundSelected,
                                    ImNodesCol_NodeOutline,
                                    ImNodesCol_TitleBar,
                                    ImNodesCol_TitleBarHovered,
                                    ImNodesCol_TitleBarSelected,
                                    ImNodesCol_Pin,
                                    ImNodesCol_PinHovered,
                                    ImNodesCol_BoxSelector,
                                    ImNodesCol_BoxSelectorOutline,
                                    ImGuiCol_Text);

        ImNodes::BeginNode(nodeUid);
        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted(node.GetNodeTitle().data());
        ImNodes::EndNodeTitleBar();

        const auto& allInPorts = node.GetInputPorts();
        const auto& allOutPorts = node.GetOutputPorts();

        std::vector<InputPort> visibleInPorts;
        std::vector<OutputPort> visibleOutPorts;
        visibleInPorts.reserve(allInPorts.size());
        visibleOutPorts.reserve(allOutPorts.size());

        // Use preference to decide whether to hide unlinked ports
        if (m_hideUnlinkedPorts)
        {
            // Hide unlinked ports mode
            for (const auto& inPort : allInPorts)
            {
                if (!inPort.HasNoEdgeLinked())
                {
                    visibleInPorts.push_back(inPort);
                }
            }

            for (const auto& outPort : allOutPorts)
            {
                if (!outPort.HasNoEdgeLinked())
                {
                    visibleOutPorts.push_back(outPort);
                }
            }
        }
        else
        {
            // Show all ports mode (original behavior)
            visibleInPorts = allInPorts;
            visibleOutPorts = allOutPorts;
        }

        const size_t inCount = visibleInPorts.size();
        const size_t outCount = visibleOutPorts.size();
        const size_t rows = std::max(inCount, outCount);

        // compute max widths
        float maxInLabelWidth  = 0.0f;
        float maxOutLabelWidth = 0.0f;
        for (const auto& inPort : visibleInPorts)
        {
            auto n = inPort.GetPortname();
            maxInLabelWidth =
                std::max(maxInLabelWidth, ImGui::CalcTextSize(n.data(), n.data() + n.size()).x);
        }

        for (const auto& outPort : visibleOutPorts)
        {
            auto n = outPort.GetPortname();
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
                const InputPort& ip = visibleInPorts[row];

                CustumiszedDrawData custumiszedDrawData{std::to_string(ip.GetPortId()), ImVec2{-15.f, -10.f},
                                                        ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]) };
                ImNodes::BeginInputAttribute(ip.GetPortUniqueId(), custumiszedDrawData);
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
                const OutputPort& op = visibleOutPorts[row];
                // Measure text width and right-align it within the output column
                auto        name  = op.GetPortname();
                const float textW = ImGui::CalcTextSize(name.data(), name.data() + name.size()).x;
                const float textPosX = outColumnX + (maxOutLabelWidth - textW);

                CustumiszedDrawData custumiszedDrawData{std::to_string(op.GetPortId()), ImVec2{10.f, -10.f},
                                                        ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text])};
                ImNodes::BeginOutputAttribute(op.GetPortUniqueId(), custumiszedDrawData);
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

void NodeEditor::ShowEdges()
{
    for (const auto& [edgeUid, edge] : m_edges)
    {
        OpacitySetter opacitySetter(edge.GetOpacity(),
                                    ImNodesCol_Link,
                                    ImNodesCol_LinkHovered,
                                    ImNodesCol_LinkSelected);

        ImNodes::Link(edgeUid, edge.GetSourcePortUid(), edge.GetDestinationPortUid());
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
    ImGui::TextUnformatted("PipelineName:");
    ImGui::SameLine();
    ImGui::TextColored(COLOR_RED, "%s", m_currentPipeLineName.c_str());
    ImGui::SameLine();
    ImGui::PopItemWidth();
    if (ImGui::SmallButton("EditName"))
    {
        ImGui::OpenPopup("PipelineNameChange");
        ImVec2 mainWindowDisplaySize = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowSize(ImVec2{mainWindowDisplaySize.x / 4, mainWindowDisplaySize.y / 4});
    }
    // pipeline name edit
    static std::string newPipeLineName{m_currentPipeLineName};
    if (ImGui::BeginPopupModal("PipelineNameChange", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("PiepelineName: ");
        ImGui::SameLine();
        if (ImGui::IsWindowAppearing())
        {
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::InputText("##pipelineNameInput", &newPipeLineName);
        ImGui::SameLine();
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

void NodeEditor::ShowPruningRuleEditWinddow(const ImVec2& mainWindowDisplaySize)
{
    ImVec2 pruningRuleEditorWindowSize{mainWindowDisplaySize.x / 4, mainWindowDisplaySize.y / 4};
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
    // ImGuiCond_Appearing must be set, otherwise the window can not be moved or resized
    ImGui::SetNextWindowPos(ImVec2(30.f, 30.f), ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(pruningRuleEditorWindowSize, ImGuiCond_Appearing);
    ImGui::Begin("PruningRuleEdit", nullptr, ImGuiWindowFlags_None);
    static bool editing            = false;
    static bool firstShowInputText = false;
    ImGui::TextUnformatted("Pruning Rules");
    ImGui::SameLine();
    if (ImGui::SmallButton("New"))
    {
        editing            = true;
        firstShowInputText = true;
    }
    ImGui::Separator();

    const auto& allPruningRules = m_pruningPolicy.GetAllPruningRules();
    const auto& currentPruningRule = m_pruningPolicy.GetCurrentPruningRule();

    for (auto& groupPair : allPruningRules)
    {
        const std::string            group    = groupPair.first;
        const std::set<std::string>& typesSet = groupPair.second;

        std::vector<std::string> types(typesSet.begin(), typesSet.end());
        std::string              treeRootNodeName{group};
        if (ImGui::TreeNodeEx(treeRootNodeName.c_str(),
                              ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Bullet))
        {
            for (size_t iterIndex = 0; iterIndex < types.size(); ++iterIndex)
            {
                auto targetIter = std::find_if(types.begin(), types.end(), 
                    [&currentPruningRule, &group](const std::string& inputStr) {
                        return currentPruningRule.at(group) == inputStr;
                    });

                size_t targetIndex = (targetIter != types.end()) ? std::distance(types.begin(), targetIter) : -1;
                bool isSelected = (iterIndex == targetIndex);

                if (ImGui::Selectable(types[iterIndex].c_str(), isSelected))
                {
                    if (currentPruningRule.at(group) != types[iterIndex])
                    {
                        SNELOG_INFO("Select a different pruning rule, change the graph  ...");
                        std::string originType{currentPruningRule.at(group)};
                        if (m_pruningPolicy.ChangePruningRule(m_nodes, m_edges, group, types[iterIndex]))
                        {

                            SNELOG_INFO("ChangePruning rule success, group {}, newtype {}", group, types[iterIndex]);
                        }
                        else
                        {
                            SNELOG_WARN("ChangePruning rule success, group {}, newtype {}", group, types[iterIndex]);
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
        if (firstShowInputText)
        {
            ImGui::SetKeyboardFocusHere();
            firstShowInputText = false;
        }
        ImGui::InputText("New Group", &newPruneGroup);
        ImGui::SameLine();
        ImGui::InputText("New Type", &newPruneType);
        ImGui::SameLine();
        ImGui::SetNextItemShortcut(ImGuiKey_Enter);
        if (ImGui::Button("Done"))
        {
            if (!m_pruningPolicy.AddNewPruningRule(newPruneGroup, newPruneType))
            {
                SNELOG_ERROR("pruning policy add new pruning rule failed");
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

ImVec2 NodeEditor::GetNodePos(NodeUniqueId nodeUid) 
{
    return ImNodes::GetNodeRect(nodeUid).Min;
}

std::optional<std::pair<std::string_view, ImVec2>> ComboFilterCombination(
    const std::string& popupName, const std::vector<std::string_view>& selectList)
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
        const ImVec2 click_pos =
            ImNodes::GetCurrentContext()->NodeEditorImgCtx->IO.MousePos; // Imnode related

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
                        SNELOG_INFO("User has selected {}, will be added in canvas", selectName);
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
    auto        ret = ComboFilterCombination("add node", {keyViews.begin(), keyViews.end()});
    if (ret)
    {
        NodeDescription selectedNodeDescription =
            s_nodeDescriptionsNameDesMap.at(ret.value().first.data());
        auto addNodeCmd = std::make_unique<AddNodeCommand>(*this, selectedNodeDescription, ret.value().second);
        ExecuteCommand(std::move(addNodeCmd));
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



NodeUniqueId NodeEditor::AddNewNodes(const NodeDescription& nodeDesc, const YamlNode& yamlNode, const NodeUniqueId nodeUid)
{
    SNE_ASSERT(yamlNode.m_nodeYamlId != -1);
    m_yamlNodeUidGenerator.RegisterUniqueID(yamlNode.m_nodeYamlId);

    Node newNode(nodeUid == -1 ? m_nodeUidGenerator.AllocUniqueID() : m_nodeUidGenerator.RegisterUniqueID(nodeUid),
                 Node::NodeType::NormalNode, yamlNode, *m_nodeStyle);

    NodeUniqueId ret = newNode.GetNodeUniqueId();

    newNode.GetInputPorts().reserve(nodeDesc.m_inputPortNames.size());
    newNode.GetOutputPorts().reserve(nodeDesc.m_outputPortNames.size());

    // add inputports in the new node (don't populate lookups yet - pointers would be invalid after move)
    for (size_t index = 0; index < nodeDesc.m_inputPortNames.size(); ++index)
    {
        InputPort newInport(m_portUidGenerator.AllocUniqueID(), (PortUniqueId)index,
                            nodeDesc.m_inputPortNames[index], newNode.GetNodeUniqueId(), (YamlPort::PortYamlId)index);
        newNode.AddInputPort(newInport);
    }

    // add outputports in the new node (don't populate lookups yet - pointers would be invalid after move)
    for (size_t index = 0; index < nodeDesc.m_outputPortNames.size(); ++index)
    {
        OutputPort newOutport(m_portUidGenerator.AllocUniqueID(), (PortUniqueId)index,
                              nodeDesc.m_outputPortNames[index], newNode.GetNodeUniqueId(), (YamlPort::PortYamlId)index);
        newNode.AddOutputPort(newOutport);
    }

    if (!m_nodes.insert({newNode.GetNodeUniqueId(), std::move(newNode)}).second)
    {
        SNELOG_ERROR("m_nodes insert new node fail! check it!");
        return -1;
    }

    // Now populate port lookups with valid pointers to ports in the stored node
    Node& storedNode = m_nodes.at(ret);
    for (const auto& port : storedNode.GetInputPorts())
    {
        m_inportPorts.emplace(port.GetPortUniqueId(),
                              storedNode.GetInputPort(port.GetPortUniqueId()));
    }

    for (const auto& port : storedNode.GetOutputPorts())
    {
        m_outportPorts.emplace(port.GetPortUniqueId(),
                               storedNode.GetOutputPort(port.GetPortUniqueId()));
    }

    return ret;
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
            auto deleteNodeCmd = std::make_unique<DeleteNodeCommand>(*this, nodeUid);
            ExecuteCommand(std::move(deleteNodeCmd));
        }
    }
}

void NodeEditor::DeleteNode(NodeUniqueId nodeUid, bool shouldUnregisterUid)
{
    if (m_nodes.count(nodeUid) == 0)
    {
        SNELOG_ERROR("deleting a nonexisting node, check it! nodeUid = {}", nodeUid);
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

void NodeEditor::HandleAddEdges()
{
    PortUniqueId startPortId, endPortId;

    // note : edge is linked from startPortId to endPortId
    if (ImNodes::IsLinkCreated(&startPortId, &endPortId))
    {
        auto addEdgeCmd = std::make_unique<AddEdgeCommand>(*this, startPortId, endPortId);
        ExecuteCommand(std::move(addEdgeCmd));
    }
}

bool IsInportAlreadyHasEdge(PortUniqueId portUid, std::unordered_map<PortUniqueId, InputPort*>& inports)
{
    auto iter = inports.find(portUid);
    if (iter != inports.end())
    {
        InputPort* inport = iter->second;
        if (inport->GetEdgeUid() != -1)
        {
            SNELOG_WARN("inportportuid:{} already has an edge, edgeuid:{}", portUid, inport->GetEdgeUid());
            return true;
        }
    }
    else
    {
        SNELOG_ERROR("can not find inport, portid = {}, checkit!", portUid);
    }
    return false;
}

void FillYamlEdgePort(YamlPort& yamlPort, const Port& port, std::unordered_map<NodeUniqueId, Node>& nodes)
{
    const Node& node      = nodes.at(port.GetOwnedNodeUid());
    yamlPort.m_nodeName   = node.GetNodeTitle();
    yamlPort.m_nodeYamlId = node.GetYamlNode().m_nodeYamlId;
    yamlPort.m_portName   = port.GetPortname();
    yamlPort.m_portYamlId = port.GetPortYamlId();
}

EdgeUniqueId NodeEditor::AddNewEdge(PortUniqueId srcPortUid, PortUniqueId dstPortUid,
                            const YamlEdge& yamlEdge, bool avoidMultipleInputLinks)
{
    // avoid multiple edges linking to the same inport
    if (avoidMultipleInputLinks && IsInportAlreadyHasEdge(dstPortUid, m_inportPorts))
    {
        SNELOG_WARN("inport port can not have multiple edges, inportUid[{}] portName[{}]",
                    dstPortUid, m_inportPorts.at(dstPortUid)->GetPortname().data());
        return -1;
    }

    Edge newEdge(srcPortUid, dstPortUid, m_edgeUidGenerator.AllocUniqueID(), yamlEdge);

    // set inportport's edgeid
    if (m_inportPorts.count(dstPortUid) != 0 && m_inportPorts[dstPortUid] != nullptr)
    {
        InputPort& inputPort = *m_inportPorts[dstPortUid];
        inputPort.SetEdgeUid(newEdge.GetEdgeUniqueId());
        newEdge.SetDestinationNodeUid(inputPort.GetOwnedNodeUid());
        if (!yamlEdge.m_isValid)
        {
            FillYamlEdgePort(newEdge.GetYamlEdge().m_yamlDstPort, inputPort, m_nodes);
        }
        m_pruningPolicy.SyncPruningRuleBetweenNodeAndEdge(m_nodes.at(inputPort.GetOwnedNodeUid()), newEdge);
    }
    else
    {
        SNELOG_ERROR(
            "not find input port or the pointer is nullptr when handling new edges, check it! "
            "dstPortUid is {}",
            dstPortUid);
    }

    // set outportport's edgeid
    if (m_outportPorts.count(srcPortUid) != 0 && m_outportPorts[srcPortUid] != nullptr)
    {
        OutputPort& outpurPort = *m_outportPorts[srcPortUid];
        outpurPort.PushEdge(newEdge.GetEdgeUniqueId());
        newEdge.SetSourceNodeUid(outpurPort.GetOwnedNodeUid());
        if (!yamlEdge.m_isValid)
        {
            FillYamlEdgePort(newEdge.GetYamlEdge().m_yamlSrcPort, outpurPort, m_nodes);
        }
        m_pruningPolicy.SyncPruningRuleBetweenNodeAndEdge(m_nodes.at(outpurPort.GetOwnedNodeUid()), newEdge);
    }
    else
    {
        SNELOG_ERROR(
            "not find output port or the pointer is nullptr when handling new edges, check it! "
            "srcPortUid is {}",
            srcPortUid);
    }
    DumpEdge(newEdge);
    newEdge.GetYamlEdge().m_isValid = true;
    m_edges.emplace(newEdge.GetEdgeUniqueId(), (newEdge));
    if (m_pruningPolicy.ApplyCurrentPruningRule(m_nodes, m_edges))
    {
        SNELOG_INFO( "AddNew Edge and apply pruning rule successfully");
    }
    return newEdge.GetEdgeUniqueId();
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
                auto deleteEdgeCmd = std::make_unique<DeleteEdgeCommand>(*this, edgeUid);
                ExecuteCommand(std::move(deleteEdgeCmd));
            }
            else
            {
                SNELOG_ERROR("cannot find edge in m_edges, edgeUid = {}, check it!", edgeUid);
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
            auto deleteEdgeCmd = std::make_unique<DeleteEdgeCommand>(*this, detachedEdgeUId);
            ExecuteCommand(std::move(deleteEdgeCmd));
        }
        else
        {
            SNELOG_ERROR("cannot find edge in m_edges, detachedEdgeUId = {}, check it!",
                         detachedEdgeUId);
        }
        SNELOG_INFO("dropped link id{}", detachedEdgeUId);
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
    //         SNELOG_ERROR("cannot find edge in m_edges, dropedEdgeUId = {}, check it!",
    //         dropedEdgeUId);
    //     }
    //     SNELOG_INFO("dropped link id{}", dropedEdgeUId);
    // }
}

void NodeEditor::DeleteEdge(EdgeUniqueId edgeUid, bool shouldUnregisterUid)
{
    if (m_edges.count(edgeUid) == 0)
    {
        SNELOG_WARN("try to delte a non-exist edgeUid[{}]", edgeUid);
        return;
    }
    DeleteEdgeUidFromPort(edgeUid);

    if (shouldUnregisterUid)
    {
        m_edgeUidGenerator.UnregisterUniqueID(edgeUid);
    }

    m_edges.erase(edgeUid);
}

void NodeEditor::DeleteEdgesBeforDeleteNode(NodeUniqueId nodeUid, bool shouldUnregisterUid)
{
    if (m_nodes.count(nodeUid) == 0)
    {
        SNELOG_ERROR("handling an nonexisting node, please check it! nodeUid = {}", nodeUid);
        return;
    }

    Node& node = m_nodes.at(nodeUid);
    for (InputPort& inPort : node.GetInputPorts())
    {
        if (inPort.GetEdgeUid() != -1)
        {
            DeleteEdge(inPort.GetEdgeUid(), shouldUnregisterUid);
            SNELOG_INFO("nodeUid {} inportUid {} deleteEdgeUid {}", node.GetNodeUniqueId(),
                        inPort.GetPortUniqueId(), inPort.GetEdgeUid());
        }
    }

    for (OutputPort& outPort : node.GetOutputPorts())
    {
        const std::vector<EdgeUniqueId> edgeUids = outPort.GetEdgeUids();
        for (EdgeUniqueId edgeUid : edgeUids)
        {
            DeleteEdge(edgeUid, shouldUnregisterUid);
            SNELOG_INFO("nodeUid {} outportUid {} deleteEdgeUid {}", node.GetNodeUniqueId(),
                        outPort.GetPortUniqueId(), edgeUid);
        }
    }

    // check that we have already deleted all edges from Node
    for (InputPort& inPort : node.GetInputPorts())
    {
        SNE_ASSERT(inPort.GetEdgeUid() == -1, "inport's edge still did not deleted");
    }

    for (OutputPort& outPort : node.GetOutputPorts())
    {
        SNE_ASSERT(outPort.GetEdgeUids().size() == 0, "outport's edge are not deleted");
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
            SNELOG_ERROR("cannot find inports in m_outportPorts inportUid = {}",
                         edge.GetSourcePortUid());
        }

        if (m_inportPorts.count(edge.GetDestinationPortUid()))
        {
            m_inportPorts[edge.GetDestinationPortUid()]->SetEdgeUid(-1);
        }
        else
        {
            SNELOG_ERROR("cannot find inports in m_inportPorts outputid = {}",
                         edge.GetDestinationPortUid());
        }
    }
    else
    {
        SNELOG_ERROR("cannot find edge in m_edges, edgeUid = {}, check it!", edgeUid);
    }
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
            SNELOG_INFO("nodeuid[{}], nodewidth[{}]", nodeUid,
                        ImNodes::GetNodeRect(nodeUid).GetWidth());
            columHeight += (ImNodes::GetNodeRect(nodeUid).GetHeight() + verticalPadding);
        }
        columWidths.push_back(columWidth);
        columHeights.push_back(columHeight);
    }

    SNE_ASSERT(topologicalOrder.size() == columWidths.size() &&
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


void NodeEditor::HandleNodeInfoEditing()
{
    static NodeUniqueId nodeUidToBePoped{-1};
    static YamlNode     popUpYamlNode{};
    NodeUniqueId        selectedNode{-1};
    // handle userinteractions to prepare for popup
    if (ImNodes::IsNodeHovered(&selectedNode) && ImGui::IsMouseDoubleClicked(0))
    {
        SNE_ASSERT(selectedNode != -1);
        SNELOG_INFO("node with nodeUid[{}] has been double clicked", selectedNode);

        nodeUidToBePoped = selectedNode;
        // Initialize popup buffers from the node's current YAML data
        if (m_nodes.contains(selectedNode))
        {
            const YamlNode& yn = m_nodes.at(selectedNode).GetYamlNode();
            popUpYamlNode      = yn;
            ImGui::OpenPopup("Node Info Editor");
        }
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetColorU32(ImVec4(0.12f, 0.12f, 0.12f, 0.85f)));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImVec4(0.95f, 0.95f, 0.95f, 1.0f)));

    ImVec2   mainWindowDisplaySize = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowSize(ImVec2(mainWindowDisplaySize.x / 3, mainWindowDisplaySize.y / 3), ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(
            "Node Info Editor", nullptr,
             ImGuiWindowFlags_HorizontalScrollbar))
    {

        ImGui::TextColored(COLOR_VISIBLE, "BasicInfos");
        ImGui::Separator();
        ImGui::Text("NodeUid: %d", nodeUidToBePoped);
        ImGui::Text("NodeName: ");
        ImGui::SameLine();
        ImGui::InputText("##NodeName", &popUpYamlNode.m_nodeName);
        ImGui::Text("YamlId: %d", popUpYamlNode.m_nodeYamlId);
        ImGui::Text("YamlType: %d", popUpYamlNode.m_nodeYamlType);
        ImGui::Checkbox("IsSource ", reinterpret_cast<bool*>(&popUpYamlNode.m_isSrcNode));
        ImGui::Separator();

        ImGui::TextColored(COLOR_VISIBLE, "Properties");
        ImGui::SameLine();
        if (ImGui::SmallButton("AddNewProperty"))
        {
            ImGui::OpenPopup("AddNodeProperty");
            SNELOG_INFO("Add New Node Property, open popup...");
        }
        // show node property
        if (!popUpYamlNode.m_Properties.empty())
        {
            if (ImGui::BeginTable("PropertiesTable", 3,
                                  ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings |
                                      ImGuiTableFlags_Borders))
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
                    ImGui::InputText(prop_name_label.c_str(),
                                     &popUpYamlNode.m_Properties[i].m_propertyName,
                                     ImGuiInputTextFlags_CharsNoBlank);

                    ImGui::TableSetColumnIndex(1);
                    const std::string prop_id_label = std::string("##prop_id_") + std::to_string(i);
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::InputInt(prop_id_label.c_str(), &popUpYamlNode.m_Properties[i].m_propertyId);

                    ImGui::TableSetColumnIndex(2);
                    const std::string prop_value_label = std::string("##prop_val_") + std::to_string(i);
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::InputText(prop_value_label.c_str(),
                                     &popUpYamlNode.m_Properties[i].m_propertyValue);
                }
                ImGui::EndTable();
            }
        }
        else
        {
            ImGui::TextDisabled("No properties");
        }

        // draw add node property popup
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing);
        if (ImGui::BeginPopupModal("AddNodeProperty", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::PushItemWidth(70.f);
            YamlNodeProperty newProperty;
            ImGui::InputText("propertyName", &newProperty.m_propertyName);
            ImGui::SameLine();
            ImGui::InputInt("propertyId", &newProperty.m_propertyId);
            ImGui::SameLine();
            ImGui::InputText("propertyValue", &newProperty.m_propertyValue);
            ImGui::SameLine();
            ImGui::PopItemWidth();

            ImGui::PushItemWidth(50.f);
            if (ImGui::Button("Done"))
            {
                popUpYamlNode.m_Properties.push_back(std::move(newProperty));
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            ImGui::SetNextItemShortcut(ImGuiKey_Escape);
            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopItemWidth();
            ImGui::EndPopup();
        }

        // button for add new pruning rule
        ImGui::Separator();
        ImGui::TextColored(COLOR_VISIBLE, "PruningRules");
        ImGui::SameLine();
        if (ImGui::SmallButton("AddNewRule"))
        {
            if (!m_pruningPolicy.GetAllPruningRules().empty())
            {
                SPDLOG_INFO("open popup addnode pruning rule");
                ImGui::OpenPopup("AddNodePruningRule");
            }
            else
            {
                SPDLOG_WARN("Please Add Pruning Rule first");
                Notifier::Add(Message(Message::Type::WARNING, "", "Please Add Pruning Rule first"));
            }
        }

        // show PruningRules
        auto shouldBeDeleted = popUpYamlNode.m_PruningRules.end();
        for (auto iter = popUpYamlNode.m_PruningRules.begin();
             iter != popUpYamlNode.m_PruningRules.end(); ++iter)
        {
            ImGui::PushItemWidth(50.f);
            ImGui::TextUnformatted(iter->m_Group.c_str());
            ImGui::SameLine();
            ImGui::TextUnformatted(iter->m_Type.c_str());
            ImGui::SameLine();
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
        // Modal popup: choose a group/type from m_allPruningRules
        // add new pruning rule
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing);
        if (ImGui::BeginPopupModal("AddNodePruningRule", nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize))
        {
            std::vector<std::string_view> groups{(m_pruningPolicy.GetAllPruningRules() | std::views::keys).begin(),
                                                 (m_pruningPolicy.GetAllPruningRules() | std::views::keys).end()};
            static size_t                 selectedGroupIndex = 0;
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

            const auto& typeSet = m_pruningPolicy.GetAllPruningRules().at(groups[selectedGroupIndex].data());
            std::vector<std::string_view> types{typeSet.begin(), typeSet.end()};
            static size_t                 selectedTypeIndex = 0;
            // types for selected group
            if (ImGui::BeginCombo("Type", types[selectedTypeIndex].data()))
            {
                for (size_t i = 0; i < types.size(); ++i)
                {
                    bool is_sel = (selectedTypeIndex == i);
                    if (ImGui::Selectable(types[i].data(), is_sel)) selectedTypeIndex = i;
                    if (is_sel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::Separator();

            if (ImGui::Button("Done"))
            {
                std::string_view chosenGroup =
                    groups.empty() ? std::string() : groups[selectedGroupIndex];
                std::string_view chosenType = types[selectedTypeIndex];

                if (!chosenGroup.empty() && !chosenType.empty())
                {
                    // avoid duplicates
                    bool addIt = true;
                    for (auto& [iterGroup, iterType] : popUpYamlNode.m_PruningRules)
                    {
                        if (iterGroup == chosenGroup)
                        {
                            SNELOG_WARN(
                                "This rule[Group[{}],Type[{}]] belongs to the same group may "
                                "override the original one !",
                                chosenGroup, chosenType);
                            addIt = false;
                            break;
                        }
                    }
                    if (addIt)
                        popUpYamlNode.m_PruningRules.push_back(
                            {chosenGroup.data(), chosenType.data()});
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            ImGui::SetNextItemShortcut(ImGuiKey_Escape);
            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }

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
                m_nodes.at(nodeUidToBePoped)
                    .SetNodeTitle(popUpYamlNode.m_nodeName + "_" +
                                  std::to_string(popUpYamlNode.m_nodeYamlId));
                // sync pruning rule between node and edges
                m_pruningPolicy.SyncPruningRules(m_nodes.at(nodeUidToBePoped), m_edges);
                if (m_pruningPolicy.ApplyCurrentPruningRule(m_nodes, m_edges))
                {
                    SNELOG_INFO("ApplyPruningRuleSuccess");
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

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

void NodeEditor::HandleEdgeInfoEditing()
{
    static EdgeUniqueId edgeUidToBePoped{-1};
    static YamlEdge     popUpYamlEdge{};
    EdgeUniqueId        selectedEdge{-1};

    // handle user interaction to prepare for the popup
    if (ImNodes::IsLinkHovered(&selectedEdge) && ImGui::IsMouseDoubleClicked(0))
    {
        SNE_ASSERT(selectedEdge != -1);
        SNELOG_INFO("Edge with EdgeUid[{}] has been double clicked", selectedEdge);
        edgeUidToBePoped = selectedEdge;
        if (m_edges.contains(selectedEdge))
        {
            popUpYamlEdge = m_edges.at(selectedEdge).GetYamlEdge();
            ImGui::OpenPopup("Edge Info Editor");
        }
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetColorU32(ImVec4(0.12f, 0.12f, 0.12f, 0.85f)));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImVec4(0.95f, 0.95f, 0.95f, 1.0f)));

    ImGuiIO& io                    = ImGui::GetIO();
    ImVec2   mainWindowDisplaySize = io.DisplaySize;
    ImGui::SetNextWindowSize(ImVec2{mainWindowDisplaySize.x / 3, mainWindowDisplaySize.y / 3}, ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(
            "Edge Info Editor", nullptr,
            ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGui::TextColored(COLOR_VISIBLE, "BasicInfos");
        ImGui::Text("Edge UID: %d", edgeUidToBePoped);
        ImGui::Separator();

        // Source Port (read-only)
        ImGui::TextColored(COLOR_VISIBLE, "Source Port Information");
        ImGui::Text("NodeId: %d", popUpYamlEdge.m_yamlSrcPort.m_nodeYamlId);
        ImGui::TextUnformatted(
            (std::string("NodeName: ") + popUpYamlEdge.m_yamlSrcPort.m_nodeName).c_str());
        ImGui::Text("PortId: %d", popUpYamlEdge.m_yamlSrcPort.m_portYamlId);
        ImGui::TextUnformatted(
            (std::string("PortName: ") + popUpYamlEdge.m_yamlSrcPort.m_portName).c_str());

        ImGui::Separator();

        // Destination Port (read-only)
        ImGui::TextColored(COLOR_VISIBLE, "Destination Port Information");
        ImGui::Text("NodeId: %d", popUpYamlEdge.m_yamlDstPort.m_nodeYamlId);
        ImGui::TextUnformatted(
            (std::string("NodeName: ") + popUpYamlEdge.m_yamlDstPort.m_nodeName).c_str());
        ImGui::Text("PortId: %d", popUpYamlEdge.m_yamlDstPort.m_portYamlId);
        ImGui::TextUnformatted(
            (std::string("PortName: ") + popUpYamlEdge.m_yamlDstPort.m_portName).c_str());

        ImGui::Separator();
        ImGui::TextColored(COLOR_VISIBLE, "DestinationPortPruningRules");
        // button for adding pruning rules
        ImGui::SameLine();
        if (ImGui::SmallButton("AddNew"))
        {
            if (!m_pruningPolicy.GetAllPruningRules().empty())
            {
                SPDLOG_INFO("open popup addedge pruning rule");
                ImGui::OpenPopup("AddEdgePruningRule");
            }
            else
            {
                SPDLOG_WARN("Please Add Pruning Rule first");
                Notifier::Add(Message(Message::Type::WARNING, "", "Please Add Pruning Rule first"));
            }
        }

        auto  shouldBeDeleted  = popUpYamlEdge.m_yamlDstPort.m_PruningRules.end();
        Node& srcNode          = m_nodes.at(m_edges.at(edgeUidToBePoped).GetSourceNodeUid());
        Node& dstNode          = m_nodes.at(m_edges.at(edgeUidToBePoped).GetDestinationNodeUid());

        if (popUpYamlEdge.m_yamlDstPort.m_PruningRules.size() == 0)
        {
            ImGui::TextDisabled("No pruning rules");
        }

        for (auto iter = popUpYamlEdge.m_yamlDstPort.m_PruningRules.begin();
             iter != popUpYamlEdge.m_yamlDstPort.m_PruningRules.end(); ++iter)
        {
            ImGui::PushItemWidth(50.f);
            ImGui::TextUnformatted(iter->m_Group.c_str());
            ImGui::SameLine();
            ImGui::TextUnformatted(iter->m_Type.c_str());
            // PruningRule Delete
            // onlu when the attached node does not has the pruning rule can users delete the
            // pruning rule
            if (!GetMatchedPruningRuleByGroup(*iter, srcNode.GetYamlNode().m_PruningRules) &&
                !GetMatchedPruningRuleByGroup(*iter, dstNode.GetYamlNode().m_PruningRules))
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
        // Modal popup: choose a group/type from m_allPruningRules
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing);
        if (ImGui::BeginPopupModal("AddEdgePruningRule", nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize))
        {
            std::vector<std::string_view> groups{(m_pruningPolicy.GetAllPruningRules() | std::views::keys).begin(),
                                                 (m_pruningPolicy.GetAllPruningRules() | std::views::keys).end()};
            static size_t                 selectedGroupIndex = 0;
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

            const auto& typeSet = m_pruningPolicy.GetAllPruningRules().at(groups[selectedGroupIndex].data());
            std::vector<std::string_view> types{typeSet.begin(), typeSet.end()};
            static size_t                 selectedTypeIndex = 0;
            // types for selected group
            if (ImGui::BeginCombo("Type", types[selectedTypeIndex].data()))
            {
                for (size_t i = 0; i < types.size(); ++i)
                {
                    bool is_sel = (selectedTypeIndex == i);
                    if (ImGui::Selectable(types[i].data(), is_sel)) selectedTypeIndex = i;
                    if (is_sel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::Separator();
            if (ImGui::Button("Done"))
            {
                std::string_view chosenGroup =
                    groups.empty() ? std::string() : groups[selectedGroupIndex];
                std::string_view chosenType = types[selectedTypeIndex];

                if (!chosenGroup.empty() && !chosenType.empty())
                {
                    // avoid duplicates
                    bool addIt = true;
                    for (auto& [iterGroup, iterType] : popUpYamlEdge.m_yamlDstPort.m_PruningRules)
                    {
                        if (iterGroup == chosenGroup)
                        {
                            SNELOG_WARN(
                                "This rule[Group[{}],Type[{}]] belongs to the same group may "
                                "override the original one !",
                                chosenGroup, chosenType);
                            addIt = false;
                            break;
                        }
                    }
                    if (addIt)
                        popUpYamlEdge.m_yamlDstPort.m_PruningRules.emplace_back(
                            chosenGroup.data(), chosenType.data());
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            ImGui::SetNextItemShortcut(ImGuiKey_Escape);
            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }
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
            if (ImGui::Button("CancelEdgeInfo", ImVec2(btnW, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
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

void NodeEditor::HandleOtherUserInputs()
{

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_NoPopupHierarchy |
                               ImGuiFocusedFlags_RootAndChildWindows) &&
        ImNodes::IsEditorHovered())
    {
        ImGuiIO& io = ImGui::GetIO();

        if (ImGui::IsKeyPressed(ImGuiKey_Z) && io.KeyCtrl)
        {
            if (Undo())
            {
                SNELOG_INFO("User triggered Undo via Ctrl+Z");
            }
            else
            {
                Notifier::Add(Message(Message::Type::INFO, "", "No more actions to undo!"));
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_R) && io.KeyCtrl)
        {
            if (Redo())
            {
                SNELOG_INFO("User triggered Redo via Ctrl+R");
            }
            else
            {
                Notifier::Add(Message(Message::Type::INFO, "", "No more actions to redo!"));
            }
        }

        // ctrl + S : handle saving to file, i.e serialize the pipeline to a yaml
        if (ImGui::IsKeyPressed(ImGuiKey_S) && io.KeyCtrl)
        {
            m_fileDialog.MarkFileDialogSave("untitled.yaml");
        }

        if (ImGui::IsKeyPressed(ImGuiKey_O) && io.KeyCtrl)
        {
            m_fileDialog.MarkFileDialogOpen();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_L) && io.KeyCtrl)
        {
            ClearCurrentPipeLine();
        }

        // S : toposort
        if (ImGui::IsKeyReleased(ImGuiKey_S) && !io.KeyCtrl)
        {
            m_needTopoSort = true;
        }
    }
}


bool NodeEditor::LoadPipeline(const std::string& filePath)
{
    ClearCurrentPipeLine(); // TODO: refine the logic
    if (LoadPipelineFromFile(filePath))
    {
        SNELOG_INFO("LoadPipeLineFromFile Success, filePath[{}]", filePath);
        return true;
    }
    else
    {
        ClearCurrentPipeLine();
        SNELOG_ERROR("LoadPipeLineFromFile failed, filePath[{}]", filePath);
        return false;
    }
}

bool NodeEditor::LoadPipeline(std::unique_ptr<std::istream> inputStream)
{
    ClearCurrentPipeLine();
    if (LoadPipelineFromStream(std::move(inputStream)))
    {
        SNELOG_INFO("LoadPipelineFromStream Success");
        return true;
    }
    else
    {
        ClearCurrentPipeLine();
        SNELOG_ERROR("LoadPipelineFromStream failed");
        return false;
    }
}

void NodeEditor::SetNodePos(NodeUniqueId nodeUid, const ImVec2 pos)
{
    ImNodes::SetNodeScreenSpacePos(nodeUid, pos);
}

void NodeEditor::SaveToFile(std::unique_ptr<std::ostream> outputStream)
{
    *outputStream << m_pipelineEimtter.EmitPipeline(m_currentPipeLineName, m_nodes, m_edges);
    outputStream->flush();
}

void NodeEditor::SaveToFile(const std::string& fileName)
{

    try
    {
        std::ofstream outFile(fileName);
        if (outFile.is_open())
        {
            outFile << m_pipelineEimtter.EmitPipeline(m_currentPipeLineName, m_nodes,
                                                       m_pruningPolicy.GetPrunedNodes(),
                                                       m_edges,
                                                       m_pruningPolicy.GetPrunedEdges());
            SNELOG_INFO("Successfully saved pipeline to: {}", fileName);
            outFile.close();
        }
        else
        {
            SNELOG_ERROR("Failed to open file for writing: {}", fileName);
        }
    }
    catch (const std::exception& e)
    {
        SNELOG_ERROR("Error while saving file: {}", e.what());
    }
}

// TODO: remove redundant code by combining LoadPipelineFromStream and LoadPipelineFromFile logic
bool NodeEditor::LoadPipelineFromStream(std::unique_ptr<std::istream> inputStream)
{

    bool ret = true;
    if (m_pipeLineParser.LoadStream(*inputStream))
    {
        std::vector<YamlNode> yamlNodes = m_pipeLineParser.ParseNodes();
        std::vector<YamlEdge> yamlEdges = m_pipeLineParser.ParseEdges();

        // add node in Editor
        std::unordered_map<YamlNode::NodeYamlId, NodeUniqueId> t_yamlNodeId2NodeUidMap;
        for (const YamlNode& yamlNode : yamlNodes)
        {
            if (s_nodeDescriptionsTypeDesMap.contains(yamlNode.m_nodeYamlType))
            {
                NodeUniqueId newNodeUid =
                    AddNewNodes(s_nodeDescriptionsTypeDesMap.at(yamlNode.m_nodeYamlType), yamlNode);
                t_yamlNodeId2NodeUidMap.emplace(yamlNode.m_nodeYamlId, newNodeUid);
            }
            else
            {
                ret = false;
                Notifier::Add(Message(Message::Type::ERR, "", "Invalid NodeType" + std::to_string(yamlNode.m_nodeYamlId)));
                SNE_ASSERT(false, "Invalid NodeType" + std::to_string(yamlNode.m_nodeYamlType));
                break;
            }
        }

        if (ret)
        {
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
            }

            m_pruningPolicy.CollectPruningRules(yamlNodes, yamlEdges);

            if (m_pruningPolicy.ApplyCurrentPruningRule(m_nodes, m_edges))
            {
                for (const auto& [group, type] : m_pruningPolicy.GetCurrentPruningRule())
                {
                    SNELOG_INFO(
                        "current pruning rule is : group[{}] type[{}], any node or edge that matches "
                        "the "
                        "group but not matches the type will be removed",
                        group, type);
                }
            }
            else
            {
                SNELOG_ERROR("ApplyPruningRule Fail!!");
            }

            m_needTopoSort        = true;

            m_currentPipeLineName = m_pipeLineParser.GetPipelineName();
            }
    }
    else
    {
        SNELOG_ERROR("pipelineparser loadfile failed, , check it!");
        ret = false;
    }
    return ret;
}

bool NodeEditor::LoadPipelineFromFile(const std::string& filePath)
{
    bool ret = true;

    if (m_pipeLineParser.LoadFile(filePath))
    {
        std::vector<YamlNode> yamlNodes = m_pipeLineParser.ParseNodes();
        std::vector<YamlEdge> yamlEdges = m_pipeLineParser.ParseEdges();

        // add node in Editor
        std::unordered_map<YamlNode::NodeYamlId, NodeUniqueId> t_yamlNodeId2NodeUidMap;
        for (const YamlNode& yamlNode : yamlNodes)
        {
            if (s_nodeDescriptionsTypeDesMap.contains(yamlNode.m_nodeYamlType))
            {
                NodeUniqueId newNodeUid =
                    AddNewNodes(s_nodeDescriptionsTypeDesMap.at(yamlNode.m_nodeYamlType), yamlNode);
                t_yamlNodeId2NodeUidMap.emplace(yamlNode.m_nodeYamlId, newNodeUid);
            }
            else
            {
                ret = false;
                Notifier::Add(Message(Message::Type::ERR, "", "Invalid NodeType" + std::to_string(yamlNode.m_nodeYamlId)));
                SNE_ASSERT(false, "Invalid NodeType" + std::to_string(yamlNode.m_nodeYamlType));
                break;
            }
        }

        if (ret)
        {
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

            // collect pruning rules to m_allPruningRules
            m_pruningPolicy.CollectPruningRules(yamlNodes, yamlEdges);

            if (m_pruningPolicy.ApplyCurrentPruningRule(m_nodes, m_edges))
            {
                for (const auto& [group, type] : m_pruningPolicy.GetCurrentPruningRule())
                {
                    SNELOG_INFO(
                        "current pruning rule is : group[{}] type[{}], any node or edge that matches "
                        "the "
                        "group but not matches the type will be removed",
                        group, type);
                }
            }
            else
            {
                SNELOG_ERROR("ApplyPruningRule Fail!!");
            }

            m_needTopoSort        = true;

            m_currentPipeLineName = m_pipeLineParser.GetPipelineName();
        }
    }
    else
    {
        SNELOG_ERROR("pipelineparser loadfile failed, filename is [{}], check it!", filePath);
        ret = false;
    }
    return ret;
}

void NodeEditor::ClearCurrentPipeLine()
{
    m_nodes.clear();
    m_edges.clear();
    m_inportPorts.clear();
    m_outportPorts.clear();
    m_pruningPolicy.Clear();
    m_commandQueue.Clear();
    m_portUidGenerator.Clear();
    m_nodeUidGenerator.Clear();
    m_edgeUidGenerator.Clear();
    m_pipeLineParser.Clear();
    m_pipelineEimtter.Clear();
}

void NodeEditor::ExecuteCommand(std::unique_ptr<ICommand> cmd)
{
    m_commandQueue.AddAndExecuteCommad(std::move(cmd));
    SPDLOG_INFO("ExecuteCommand done, commandqueue info: \n {}", m_commandQueue.ToString());
}


bool NodeEditor::Undo()
{
    bool ret = m_commandQueue.Undo();
    SPDLOG_INFO("UndoCommand done, commandqueue info: \n {}", m_commandQueue.ToString());
    return ret; 
}

bool NodeEditor::Redo()
{
    bool ret = m_commandQueue.Redo();
    SPDLOG_INFO("ReodCommand done, commandqueue info: \n {}", m_commandQueue.ToString());
    return ret;
}


void NodeEditor::RestoreEdge(const Edge& edgeSnapshot)
{
    SNE_ASSERT(edgeSnapshot.GetEdgeUniqueId() != -1, "edgeuid should not be -1");
    EdgeUniqueId edgeUid = edgeSnapshot.GetEdgeUniqueId();
    PortUniqueId startPortUid = edgeSnapshot.GetSourcePortUid();
    PortUniqueId endPortUid = edgeSnapshot.GetDestinationPortUid();
    
    auto startPortIt = m_outportPorts.find(startPortUid);
    auto endPortIt = m_inportPorts.find(endPortUid);
    
    if (startPortIt == m_outportPorts.end() || endPortIt == m_inportPorts.end())
    {
        SNELOG_WARN("RestoreEdge: Cannot restore edge {}: ports not found", edgeUid);
        return;
    }
    
    OutputPort* startPort = startPortIt->second;
    InputPort* endPort = endPortIt->second;
    startPort->PushEdge(edgeUid);
    endPort->SetEdgeUid(edgeUid);
    
    m_edges.emplace(edgeUid, edgeSnapshot);
    
    m_edgeUidGenerator.RegisterUniqueID(edgeUid);
}

NodeUniqueId NodeEditor::RestoreNode(const Node& nodeSnapShot)
{

    SNE_ASSERT(nodeSnapShot.GetNodeUniqueId() != -1, "nodeUid and yamlNodeUid should not be -1");
    SNELOG_INFO("Restore Node E");
    NodeUniqueId nodeUid = nodeSnapShot.GetNodeUniqueId();

    // Insert the node back into the map with its original UID
    if (!m_nodes.emplace(nodeUid, nodeSnapShot).second)
    {
        SNELOG_ERROR("RestoreNode: Failed to insert node with uid {}", nodeUid);
        return -1;
    }

    Node& restoredNode = m_nodes.at(nodeUid);
    auto& inputPorts = restoredNode.GetInputPorts();
    for (InputPort& port : inputPorts)
    {
        m_inportPorts.emplace(port.GetPortUniqueId(), restoredNode.GetInputPort(port.GetPortUniqueId()));
    }

    auto& outputPorts = restoredNode.GetOutputPorts();
    for (OutputPort& port : outputPorts)
    {
        m_outportPorts.emplace(port.GetPortUniqueId(), restoredNode.GetOutputPort(port.GetPortUniqueId()));
    }

    // Register the node UID and all port UIDs with their allocators
    m_nodeUidGenerator.RegisterUniqueID(nodeUid);
    m_yamlNodeUidGenerator.RegisterUniqueID(nodeSnapShot.GetYamlNode().m_nodeYamlId);

    for (const InputPort& port : inputPorts)
    {
        m_portUidGenerator.RegisterUniqueID(port.GetPortUniqueId());
    }

    for (const OutputPort& port : outputPorts)
    {
        m_portUidGenerator.RegisterUniqueID(port.GetPortUniqueId());
    }

    SNELOG_INFO("Restore Node X");
    SNELOG_INFO("Restore Node Info : {}", nodeSnapShot.ToString());
    return nodeUid;
}

} // namespace SimpleNodeEditor
