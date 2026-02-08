#include "GraphPruningPolicy.hpp"
#include "Log.hpp"
#include "Common.hpp"
#include <algorithm>
#include <optional>

namespace SimpleNodeEditor
{

// Helper function to find matching pruning rule by group
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

GraphPruningPolicy::GraphPruningPolicy()
    : m_allPruningRules(),
      m_currentPruningRule(),
      m_nodesPruned(),
      m_edgesPruned(),
      m_prunedOpacity(0.2f)
{ }

void GraphPruningPolicy::CollectPruningRules(const std::vector<YamlNode>& yamlNodes,
                                             const std::vector<YamlEdge>& yamlEdges)
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
        SNELOG_INFO("no pruning rule in the current yamlfile");
    }

    for (const auto& [group, type] : m_allPruningRules)
    {
        if (type.size() != 0)
        {
            if (type.size() == 1)
            {
                SNELOG_WARN("pruning rule, group[{}] only has one type!", group);
            }
            m_currentPruningRule[group] = *(type.begin());
            SNELOG_INFO("set default pruning rule, group[{}] type[{}]", group, *(type.begin()));
        }
    }
}

bool GraphPruningPolicy::AddNewPruningRule(
    const std::string& newPruningGroup, const std::string& newPruningType)
{
    if (newPruningGroup.empty() || newPruningType.empty())
    {
        return false;
    }

    if (m_allPruningRules.contains(newPruningGroup))
    {
        if (m_allPruningRules.at(newPruningGroup).contains(newPruningType))
        {
            SNELOG_WARN("try to add existed pruning rule");
            return false;
        }
        else
        {
            m_allPruningRules.at(newPruningGroup).insert(newPruningType);
        }
    }
    else
    {
        m_allPruningRules.emplace(newPruningGroup, std::set{newPruningType});
    }

    if (!m_currentPruningRule.contains(newPruningGroup))
    {
        m_currentPruningRule[newPruningGroup] = newPruningType;
    }
    return true;
}

bool GraphPruningPolicy::IsAllEdgesWillBePruned(const Node& node,
                                                const std::unordered_set<EdgeUniqueId>& shouldBeDeleteEdges) const
{
    const std::vector<EdgeUniqueId>& edges = node.GetAllEdgeUids();

    bool ret = true;
    for (EdgeUniqueId edgeUid : edges)
    {
        if (!shouldBeDeleteEdges.contains(edgeUid))
        {
            SNELOG_ERROR("NodeUid[{}] NodeYamlId[{}], still has EdgeUid[{}] not pruned", node.GetNodeUniqueId(),
                         node.GetYamlNode().m_nodeYamlId, edgeUid);
            ret = false;
        }
    }
    return ret;
}

bool GraphPruningPolicy::ChangePruningRule(std::unordered_map<NodeUniqueId, Node>& nodesMap,
                                        std::unordered_map<EdgeUniqueId, Edge>& edgesMap, const std::string& changedGroup, const std::string& changeToType)
{
    const std::string originalType{m_currentPruningRule.at(changedGroup)};
    m_currentPruningRule[changedGroup] = changeToType;

    if (ApplyCurrentPruningRule(nodesMap, edgesMap))
    {
        SNELOG_INFO("change pruning rule success, targetPruningGroup {}, targetPruningType {}, fall back to originalPruningTyep {}, restore the pruning now...", 
                    changedGroup, changeToType, originalType);
        RestorePruning(changedGroup, originalType, changeToType, nodesMap, edgesMap);
        return true;
    }
    else 
    {
        SNELOG_WARN("change pruning rule failed, targetPruningGroup {}, targetPruningType {}, fall back to originalPruningTyep {}", 
                    changedGroup, changeToType, originalType);
        m_currentPruningRule[changedGroup] = originalType;
        return false;
    }
}

bool GraphPruningPolicy::ApplyCurrentPruningRule(std::unordered_map<NodeUniqueId, Node>& nodesMap,
                                          std::unordered_map<EdgeUniqueId, Edge>& edgesMap)
{
    bool                             applyPruningRuleSuccess = true;
    std::unordered_set<NodeUniqueId> shouldBePrunedEdges;
    std::unordered_set<NodeUniqueId> shouldBePrunedNodes;

    for (const auto& [group, type] : m_currentPruningRule)
    {
        for (const auto& [edgeUid, edge] : edgesMap)
        {
            for (const auto& edgePruningRule : edge.GetYamlEdge().m_yamlDstPort.m_PruningRules)
            {
                if (edgePruningRule.m_Group == group && edgePruningRule.m_Type != type)
                {
                    auto iter = nodesMap.find(edge.GetSourceNodeUid());
                    if (iter != nodesMap.end())
                    {
                        SNELOG_INFO(
                            "Prune Edge with EdgeUid[{}] SrcNodeUid[{}] SrcNodeYamlId[{}] "
                            "DstNodeUid[{}] DstNodeYamlId[{}]",
                            edgeUid, edge.GetSourceNodeUid(),
                            edge.GetYamlEdge().m_yamlSrcPort.m_nodeYamlId,
                            edge.GetDestinationNodeUid(),
                            edge.GetYamlEdge().m_yamlDstPort.m_nodeYamlId);
                        shouldBePrunedEdges.insert(edgeUid);
                    }
                    else
                    {
                        SNELOG_ERROR("Cannot find edge's source node in nodesMap with edgeuid[{}]", edgeUid);
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
                    auto iter = nodesMap.find(nodeUid);
                    if (iter != nodesMap.end())
                    {
                        if (IsAllEdgesWillBePruned(iter->second, shouldBePrunedEdges))
                        {
                            SNELOG_INFO("Prune Node with NodeUid[{}] NodeYamlId[{}]", nodeUid,
                                        node.GetYamlNode().m_nodeYamlId);
                            shouldBePrunedNodes.insert(nodeUid);
                        }
                        else
                        {
                            SNELOG_ERROR(
                                "Not All edges of Node({}) has been pruned, please check if all "
                                "edges has the corresponding prune rule!!!",
                                nodeUid);
                            applyPruningRuleSuccess = false;
                            break;
                        }
                    }
                    else
                    {
                        SNELOG_ERROR("Cannot find node in nodesMap with nodeuid[{}]", nodeUid);
                        applyPruningRuleSuccess = false;
                        break;
                    }
                }
            }
        }
    }

    // do the pruning operation
    if (applyPruningRuleSuccess)
    {
        for (EdgeUniqueId edgeUid : shouldBePrunedEdges)
        {
            auto edgeIter = edgesMap.find(edgeUid);
            if (edgeIter != edgesMap.end())
            {
                m_edgesPruned.insert(*edgeIter);
                edgeIter->second.SetOpacity(m_prunedOpacity);
            }
        }

        for (NodeUniqueId nodeUid : shouldBePrunedNodes)
        {
            auto nodeIter = nodesMap.find(nodeUid);
            if (nodeIter != nodesMap.end())
            {
                m_nodesPruned.insert(*nodeIter);
                nodeIter->second.SetOpacity(m_prunedOpacity);
            }
        }
    }

    return applyPruningRuleSuccess;
}

void GraphPruningPolicy::RestorePruning(const std::string& changedGroup, const std::string& originType,
                                        const std::string& newType,
                                        std::unordered_map<NodeUniqueId, Node>& nodesMap,
                                        std::unordered_map<EdgeUniqueId, Edge>& edgesMap)
{
    SNE_ASSERT(originType != newType);

    SNELOG_INFO("Restoring prunerule, group[{}], originType[{}], newType[{}]", changedGroup,
                originType, newType);

    for (auto it = m_nodesPruned.begin(); it != m_nodesPruned.end();)
    {
        const NodeUniqueId nodeUid   = it->first;
        Node&              node      = it->second;
        bool               erasedOne = false;
        for (const auto& pruningRule : node.GetYamlNode().m_PruningRules)
        {
            if (pruningRule.m_Group == changedGroup && pruningRule.m_Type == newType)
            {
                SNE_ASSERT(pruningRule.m_Type != originType);
                auto nodeIter = nodesMap.find(nodeUid);
                if (nodeIter != nodesMap.end())
                {
                    nodeIter->second.SetOpacity(m_nodesPruned.find(nodeUid)->second.GetOpacity());
                    SNELOG_INFO("Restore node with nodeUid[{}], yamlNodeName[{}], yamlNodeId[{}]",
                                nodeUid, node.GetYamlNode().m_nodeName,
                                node.GetYamlNode().m_nodeYamlId);
                    it        = m_nodesPruned.erase(it);
                    erasedOne = true;
                }
            }
        }
        if (!erasedOne) ++it;
    }

    for (auto it = m_edgesPruned.begin(); it != m_edgesPruned.end();)
    {
        const EdgeUniqueId edgeUid   = it->first;
        Edge&              edge      = it->second;
        bool               erasedOne = false;
        for (const auto& pruningRule : edge.GetYamlEdge().m_yamlDstPort.m_PruningRules)
        {
            if (pruningRule.m_Group == changedGroup && pruningRule.m_Type == newType)
            {
                SNE_ASSERT(pruningRule.m_Type != originType);
                auto edgeIter = edgesMap.find(edgeUid);
                if (edgeIter != edgesMap.end())
                {
                    SNELOG_INFO(
                        "restore edge with edgeUid[{}], yamlSrcPortName[{}], yamlDstPortName[{}]",
                        edgeUid, edge.GetYamlEdge().m_yamlSrcPort.m_portName,
                        edge.GetYamlEdge().m_yamlDstPort.m_portName);
                    edgeIter->second.SetOpacity(m_edgesPruned.find(edgeUid)->second.GetOpacity());
                    erasedOne = true;
                    it        = m_edgesPruned.erase(it);
                }
            }
        }
        if (!erasedOne) ++it;
    }
}

bool GraphPruningPolicy::IsAllEdgesHasBeenPruned(NodeUniqueId nodeUid,
                                                 const std::unordered_map<NodeUniqueId, Node>& nodesMap) const
{
    auto nodeIter = nodesMap.find(nodeUid);
    if (nodeIter == nodesMap.end())
    {
        SNELOG_ERROR("Cannot find node with nodeUid[{}]", nodeUid);
        return false;
    }

    const Node& node = nodeIter->second;

    for (const auto& port : node.GetInputPorts())
    {
        if (port.GetEdgeUid() != -1)
        {
            SNELOG_ERROR("NodeUid[{}], InPortUid[{}], still has Edge ", node.GetNodeUniqueId(),
                         port.GetPortUniqueId());
            return false;
        }
    }

    for (const auto& port : node.GetOutputPorts())
    {
        if (!port.GetEdgeUids().empty())
        {
            SNELOG_ERROR("NodeUid[{}], OutPortUid[{}], still has Edge ", node.GetNodeUniqueId(),
                         port.GetPortUniqueId());
            return false;
        }
    }

    return true;
}

void GraphPruningPolicy::SyncPruningRules(const Node& node,
                                         std::unordered_map<EdgeUniqueId, Edge>& edgesMap)
{
    for (EdgeUniqueId edgeUid : node.GetAllEdgeUids())
    {
        auto edgeIter = edgesMap.find(edgeUid);
        if (edgeIter != edgesMap.end())
        {
            SyncPruningRuleBetweenNodeAndEdge(node, edgeIter->second);
        }
    }
}

void GraphPruningPolicy::SyncPruningRuleBetweenNodeAndEdge(const Node& node, Edge& edge)
{
    // any pruning rule that the node has but the edge does not have will be automatically added in
    // the yamledge if the pruning rule of node and edge are conflict, should we complain an error?
    // or shutdown the APP?       
    for (const auto& pruningRule : node.GetYamlNode().m_PruningRules)
    {                             
        auto ret = GetMatchedPruningRuleByGroup(pruningRule,
                                                edge.GetYamlEdge().m_yamlDstPort.m_PruningRules);
        if (ret)                  
        {                         
            if (ret.value()->m_Type != pruningRule.m_Type)
            {                     
                SNELOG_WARN(      
                    "Pruning rules conflicted, edge's rule will be overridden! nodeUid[{}] "
                    "nodeName[{}] "
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

const std::unordered_map<std::string, std::set<std::string>>& GraphPruningPolicy::GetAllPruningRules() const
{
    return m_allPruningRules;
}

const std::unordered_map<std::string, std::string>& GraphPruningPolicy::GetCurrentPruningRule() const
{
    return m_currentPruningRule;
}

std::unordered_map<std::string, std::string>& GraphPruningPolicy::GetCurrentPruningRule()
{
    return m_currentPruningRule;
}

const std::unordered_map<NodeUniqueId, Node>& GraphPruningPolicy::GetPrunedNodes() const
{
    return m_nodesPruned;
}

const std::unordered_map<EdgeUniqueId, Edge>& GraphPruningPolicy::GetPrunedEdges() const
{
    return m_edgesPruned;
}

void GraphPruningPolicy::Clear()
{
    m_allPruningRules.clear();
    m_currentPruningRule.clear();
    m_nodesPruned.clear();
    m_edgesPruned.clear();
}

} // namespace SimpleNodeEditor