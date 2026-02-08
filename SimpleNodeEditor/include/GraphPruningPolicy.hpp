#ifndef GRAPHPRUNINGPOLICY_H
#define GRAPHPRUNINGPOLICY_H

#include <unordered_map>
#include <unordered_set>
#include <set>
#include <string>
#include <vector>
#include "DataStructureEditor.hpp"
#include "DataStructureYaml.hpp"

namespace SimpleNodeEditor
{
// there is only one pruning rule now:
// when the node or edge is pruned, opacity will be set to 0(or some value that close to 0)
// i.e. the node or the edge will not be pruned actually
class GraphPruningPolicy
{
public:
    GraphPruningPolicy();
    ~GraphPruningPolicy() = default;

    GraphPruningPolicy(const GraphPruningPolicy&) = delete;
    GraphPruningPolicy& operator=(const GraphPruningPolicy&) = delete;
    GraphPruningPolicy(GraphPruningPolicy&&) = default;
    GraphPruningPolicy& operator=(GraphPruningPolicy&&) = default;

    void CollectPruningRules(const std::vector<YamlNode>& yamlNodes,
                            const std::vector<YamlEdge>& yamlEdges);

    bool AddNewPruningRule(const std::string& newPruningGroup,
                          const std::string& newPruningType);

    bool ApplyCurrentPruningRule(std::unordered_map<NodeUniqueId, Node>& nodesMap,
                                        std::unordered_map<EdgeUniqueId, Edge>& edgesMap);
    bool ChangePruningRule(std::unordered_map<NodeUniqueId, Node>& nodesMap,
                                        std::unordered_map<EdgeUniqueId, Edge>& edgesMap,
                                        const std::string& changedGroup, const std::string& changeToType);

    void RestorePruning(const std::string& changedGroup,
                       const std::string& originType,
                       const std::string& newType,
                       std::unordered_map<NodeUniqueId, Node>& nodesMap,
                       std::unordered_map<EdgeUniqueId, Edge>& edgesMap);

    void SyncPruningRules(const Node& node,
                         std::unordered_map<EdgeUniqueId, Edge>& edgesMap);

    void SyncPruningRuleBetweenNodeAndEdge(const Node& node, Edge& edge);

    bool IsAllEdgesHasBeenPruned(NodeUniqueId nodeUid,
                                const std::unordered_map<NodeUniqueId, Node>& nodesMap) const;

    const std::unordered_map<std::string, std::set<std::string>>& GetAllPruningRules() const;
    const std::unordered_map<std::string, std::string>& GetCurrentPruningRule() const;
    std::unordered_map<std::string, std::string>& GetCurrentPruningRule();
    const std::unordered_map<NodeUniqueId, Node>& GetPrunedNodes() const;
    const std::unordered_map<EdgeUniqueId, Edge>& GetPrunedEdges() const;

    void Clear();

private:
    bool IsAllEdgesWillBePruned(const Node& node,
                               const std::unordered_set<EdgeUniqueId>& shouldBeDeleteEdges) const;
    std::unordered_map<std::string, std::set<std::string>> m_allPruningRules;
    std::unordered_map<std::string, std::string> m_currentPruningRule;
    std::unordered_map<NodeUniqueId, Node> m_nodesPruned;
    std::unordered_map<EdgeUniqueId, Edge> m_edgesPruned;
    float m_prunedOpacity;
};

} // namespace SimpleNodeEditor

#endif // GRAPHPRUNINGPOLICY_H