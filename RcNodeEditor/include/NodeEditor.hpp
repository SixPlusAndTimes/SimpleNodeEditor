#ifndef NODEEDITOR_H
#define NODEEDITOR_H
#include "Node.hpp"
#include "imnodes.h"
#include "NodeDescription.hpp"
#include <unordered_set>
#include <set>
#include "Helpers.h"
#include "YamlParser.hpp"
#include "YamlEmitter.hpp"

struct ImNodesStyle;

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
    void HandleFileDrop(const std::string& filePath);

private:
    void         DrawMenu();
    void         MenuStyle();
    void         MenuFile();
    void         HandleAddNodes();
    NodeUniqueId AddNewNodes(const NodeDescription& nodeDesc);
    NodeUniqueId AddNewNodes(const NodeDescription& nodeDesc, const YamlNode& yamlNde);
    void         DeleteNode(NodeUniqueId nodeUid, bool shouldUnregisterUid);
    void         ShowNodes();
    void         ShowEdges();
    void         HandleAddEdges();
    void         ShowPipelineName();

    void FillYamlEdgePort(YamlPort& yamlPort, const Port& port);

    void AddNewEdge(PortUniqueId srcPortUid, PortUniqueId dstPortUid, const YamlEdge& yamlEdge = {},
                    bool avoidMultipleInputLinks = true);
    void HandleDeletingEdges();
    void DeleteEdge(EdgeUniqueId edgeUid, bool shouldUnregisterUid);
    void DeleteEdgesBeforDeleteNode(NodeUniqueId nodeUid, bool shouldUnregisterUid);
    [[nodiscard]] bool IsInportAlreadyHasEdge(PortUniqueId portUid);
    void               DeleteEdgeUidFromPort(EdgeUniqueId edgeUid);

    void SaveState();
    void SaveToFile();
    void RearrangeNodesLayout(const std::vector<std::vector<NodeUniqueId>>& topologicalOrder,
                              const std::unordered_map<NodeUniqueId, Node>& nodesMap);
    void CollectPruningRules(std::vector<YamlNode> yamlNodes, std::vector<YamlEdge> yamlEdges);

    bool IsAllEdgesWillBePruned(NodeUniqueId nodeUid, const std::unordered_set<EdgeUniqueId>& shouldBeDeleteEdges);

    [[nodiscard]] bool ApplyPruningRule(const std::unordered_map<std::string, std::string>& currentPruningRule,
                                  std::unordered_map<NodeUniqueId, Node>       nodesMap,
                                  std::unordered_map<EdgeUniqueId, Edge>       edgesMap);
    void RestorePruning(const std::string& group, const std::string& orignType,
                        const std::string& newType);

    void ShowGrapghEditWindow(const ImVec2& mainWindowDisplaySize);
    void ShowPruningRuleEditWinddow(const ImVec2& mainWindowDisplaySize);

    [[nodiscard]] bool IsAllEdgesHasBeenPruned(NodeUniqueId nodeUid);

    [[nodiscard]] bool LoadPipelineFromFile(const std::string& filePath);
    void               ClearCurrentPipeLine();

    void SyncPruningRuleBetweenNodeAndEdge(const Node& node, Edge& edge);
    void HandleOtherUserInputs();
    void HandleNodeInfoEditing();
    void HandleEdgeInfoEditing();

private:
    std::unordered_map<NodeUniqueId, Node> m_nodes; // store nodes that will be rendered on canvas

    std::unordered_map<EdgeUniqueId, Edge> m_edges; // store edges that will be rendered on canvas

    std::unordered_map<PortUniqueId, InputPort*>
        m_inportPorts; // hold pointers to ports which actually owned by Nodes
    std::unordered_map<PortUniqueId, OutputPort*> m_outportPorts;

    // uniqueid generators
    UniqueIdAllocator<NodeUniqueId> m_nodeUidGenerator;
    UniqueIdAllocator<PortUniqueId> m_portUidGenerator;
    UniqueIdAllocator<EdgeUniqueId> m_edgeUidGenerator;
    
    UniqueIdAllocator<YamlNode::NodeYamlId> m_yamlNodeUidGenerator;

    ImNodesMiniMapLocation m_minimap_location;

    std::vector<NodeDescription> m_nodeDescriptions;

    bool m_needTopoSort;

    std::unordered_map<std::string, std::set<std::string>> m_allPruningRules;
    // key : group, value : type
    std::unordered_map<std::string, std::string> m_currentPruninngRule;

    std::unordered_map<NodeUniqueId, Node>
        m_nodesPruned; // store nodes that will be rendered on canvas

    std::unordered_map<EdgeUniqueId, Edge>
        m_edgesPruned; // store edges that will be rendered on canvas

    std::string m_currentPipeLineName;

    ImNodesStyle& m_nodeStyle;

    PipelineParser m_pipeLineParser;
    PipelineEmitter m_pipelineEimtter;
    
};
} // namespace SimpleNodeEditor

#endif // NODEEDITOR_H