#ifndef NODEEDITOR_H
#define NODEEDITOR_H
#include "Node.hpp"
#include "imnodes.h"
#include "NodeDescription.hpp"
#include <set>

namespace SimpleNodeEditor
{

class NodeEditor
{
public:
    NodeEditor();
    void NodeEditorInitialize();
    void NodeEditorShow();
    void NodeEditorDestroy();
    void HandleDeletingNodes();

private:
    void         ShowMenu();
    void         ShowInfos();
    void         HandleAddNodes();
    NodeUniqueId AddNewNodes(const NodeDescription& nodeDesc, const YamlNode& yamlNode = {});
    void         DeleteNode(NodeUniqueId nodeUid);
    void         ShowNodes();
    void         ShowEdges();
    void         HandleAddEdges();
    void         AddNewEdge(PortUniqueId srcPortUid, PortUniqueId dstPortUid, const YamlEdge& yamlEdge = {}, bool avoidMultipleInputLinks = true);
    void         HandleDeletingEdges();
    void         DeleteEdge(EdgeUniqueId edgeUid);
    void         DeleteEdgesBeforDeleteNode(NodeUniqueId nodeUid);
    bool         IsInportAlreadyHasEdge(PortUniqueId portUid);
    void         DeleteEdgeUidFromPort(EdgeUniqueId edgeUid);
    void         SaveState();
    void RearrangeNodesLayout(const std::vector<std::vector<NodeUniqueId>>& topologicalOrder,
                              const std::unordered_map<NodeUniqueId, Node>& nodesMap);
    void CollectPruningRules(std::vector<YamlNode> yamlNodes, std::vector<YamlEdge> yamlEdges);

    void ApplyPruningRule(std::unordered_map<std::string, std::string> currentPruningRule, std::unordered_map<NodeUniqueId, Node> nodesMap, std::unordered_map<EdgeUniqueId, Edge> edgesMap);
    void RestorePruning(const std::string& group, const std::string& orignType, const std::string& newType);

    void ShowPruningRuleEditWinddow(const ImVec2& mainWindowDisplaySize);
    void HandleOtherUserInputs();

private:
    std::unordered_map<NodeUniqueId, Node> m_nodes; // store nodes that will be rendered on canvas

    std::unordered_map<EdgeUniqueId, Edge> m_edges; // store edges that will be rendered on canvas

    std::unordered_map<PortUniqueId, InputPort*>
        m_inportPorts; // hold pointers to ports which actually owned by Nodes
    std::unordered_map<PortUniqueId, OutputPort*> m_outportPorts;

    // uniqueid generators
    UniqueIdGenerator<NodeUniqueId> m_nodeUidGenerator;
    UniqueIdGenerator<PortUniqueId> m_portUidGenerator;
    UniqueIdGenerator<EdgeUniqueId> m_edgeUidGenerator;

    ImNodesMiniMapLocation m_minimap_location;

    std::vector<NodeDescription> m_nodeDescriptions;

    bool m_needTopoSort;
    
    // store all pruning rules
    // key : group 
    // value : set of type
    std::unordered_map<std::string, std::set<std::string>> m_allPruningRules;
    // key : group, value : type
    std::unordered_map<std::string, std::string> m_currentPruninngRule;
    
    std::unordered_map<NodeUniqueId, Node> m_nodesPruned; // store nodes that will be rendered on canvas

    std::unordered_map<EdgeUniqueId, Edge> m_edgesPruned; // store edges that will be rendered on canvas

};
} // namespace SimpleNodeEditor

#endif // NODEEDITOR_H