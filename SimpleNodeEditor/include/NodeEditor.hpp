#ifndef NODEEDITOR_H
#define NODEEDITOR_H
#include "DataStructureEditor.hpp"
#include "DataStructureYaml.hpp"
#include "Helpers.hpp"
#include "YamlParser.hpp"
#include "YamlEmitter.hpp"
#include "FileDialog.hpp"
#include <unordered_set>
#include "imnodes.h"
#include <set>

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
    bool OpenFile(const std::string& filePath);

private:
    // draw ui infereface
    void DrawMenu();
    void DrawFileMenu();
    void DrawFileDialog();
    void ShowNodes();
    void ShowEdges();
    void ShowPipelineName();
    void ShowGrapghEditWindow(const ImVec2& mainWindowDisplaySize);
    void ShowPruningRuleEditWinddow(const ImVec2& mainWindowDisplaySize);

    // handle add/delete nodes
    void         HandleAddNodes();
    NodeUniqueId AddNewNodes(const NodeDescription& nodeDesc);
    NodeUniqueId AddNewNodes(const NodeDescription& nodeDesc, const YamlNode& yamlNde);
    void         HandleDeletingNodes();
    void         DeleteNode(NodeUniqueId nodeUid, bool shouldUnregisterUid);

    // handle add/delete edges
    void HandleAddEdges();
    void AddNewEdge(PortUniqueId srcPortUid, PortUniqueId dstPortUid, const YamlEdge& yamlEdge = {},
                    bool avoidMultipleInputLinks = true);
    void               HandleDeletingEdges();
    void               DeleteEdge(EdgeUniqueId edgeUid, bool shouldUnregisterUid);
    void               DeleteEdgesBeforDeleteNode(NodeUniqueId nodeUid, bool shouldUnregisterUid);
    void               DeleteEdgeUidFromPort(EdgeUniqueId edgeUid);
    // handle nodes layout afer toposorted
    void RearrangeNodesLayout(const std::vector<std::vector<NodeUniqueId>>& topologicalOrder,
                              const std::unordered_map<NodeUniqueId, Node>& nodesMap);
    // pruning rule related
    void CollectPruningRules(std::vector<YamlNode> yamlNodes, std::vector<YamlEdge> yamlEdges);

    bool AddNewPruningRule(const std::string& newPruningGroup, const std::string& newPruningType,
                           std::unordered_map<std::string, std::set<std::string>>& allPruningRule);
    [[nodiscard]] bool ApplyPruningRule(
        const std::unordered_map<std::string, std::string>& currentPruningRule,
        std::unordered_map<NodeUniqueId, Node>              nodesMap,
        std::unordered_map<EdgeUniqueId, Edge>              edgesMap);
    void               RestorePruning(const std::string& group, const std::string& orignType,
                                      const std::string& newType);
    [[nodiscard]] bool IsAllEdgesHasBeenPruned(NodeUniqueId nodeUid);
    void               SyncPruningRules(const Node& node);
    void               SyncPruningRuleBetweenNodeAndEdge(const Node& node, Edge& edge);

    // handle user interactions
    void HandleNodeInfoEditing();
    void HandleEdgeInfoEditing();
    void HandleZooming();
    void HandleOtherUserInputs();

    void               SaveToFile(const std::string& fileName); 
    void               SaveToFile(std::unique_ptr<std::ostream> outputStream);
    [[nodiscard]] bool LoadPipelineFromFile(const std::string& filePath);
    void               ClearCurrentPipeLine();

private:
    // maybe map is better than unorderedmap, 
    // because so many insert/delete operations contributing to growing memory space of hashmap 
    // which may never shrink to a suiatable volume
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

    // should held by nodeeditor?
    std::vector<NodeDescription> m_nodeDescriptions;

    bool m_needTopoSort;
    // TODO : may hold the latest toposort res

    // all pruning rule that colleceted from exsiting yamlfile or added by user
    std::unordered_map<std::string, std::set<std::string>> m_allPruningRules;
    // the current prunging value that applied to the the nodes and edges
    // all pruned nodes or edges are stored in m_nodesPruned and m_edgesPruned
    std::unordered_map<std::string, std::string> m_currentPruninngRule;
    std::unordered_map<NodeUniqueId, Node>
        m_nodesPruned; // store nodes that will not be rendered on canvas
    std::unordered_map<EdgeUniqueId, Edge>
        m_edgesPruned; // store edges that will not be rendered on canvas

    std::string m_currentPipeLineName;

    ImNodesStyle& m_nodeStyle;

    // serilizer and deserializer
    // should held by nodeeditor instance?
    PipelineEmitter m_pipelineEimtter;
    PipelineParser  m_pipeLineParser;

    FileDialog      m_fileDialog;
};
} // namespace SimpleNodeEditor

#endif // NODEEDITOR_H