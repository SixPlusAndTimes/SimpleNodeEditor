#ifndef NODEEDITOR_H
#define NODEEDITOR_H
#include "DataStructureEditor.hpp"
#include "DataStructureYaml.hpp"
#include "Helpers.hpp"
#include "YamlParser.hpp"
#include "YamlEmitter.hpp"
#include "FileDialog.hpp"
#include "GraphPruningPolicy.hpp"
#include <unordered_set>
#include "imnodes.h"
#include <set>
#include "CommandQueue.hpp"


struct ImNodesStyle;

namespace SimpleNodeEditor
{

class NodeEditor
{
    friend ICommand;
    friend AddEdgeCommand;
    friend AddNodeCommand;
    friend DeleteEdgeCommand;
    friend DeleteNodeCommand;
public:
    NodeEditor();
    void NodeEditorInitialize();
    void NodeEditorShow();
    void NodeEditorDestroy();
    bool LoadPipeline(const std::string& filePath);
    bool LoadPipeline(std::unique_ptr<std::istream> inputStream);
    void SetNodePos(NodeUniqueId nodeUid, const ImVec2 pos);

public: // TODO: private
    // draw ui infereface
    void DrawMenu();
    void DrawFileMenu();
    void DrawConfig();
    void DrawFileDialog();
    void ShowNodes();
    void ShowEdges();
    void ShowPipelineName();
    void ShowGrapghEditWindow(const ImVec2& mainWindowDisplaySize);
    void ShowPruningRuleEditWinddow(const ImVec2& mainWindowDisplaySize);
    //
    ImVec2 GetNodePos(NodeUniqueId nodeUid);

    // handle add/delete nodes
    void         HandleAddNodes();
    NodeUniqueId AddNewNodes(const NodeDescription& nodeDesc);
    NodeUniqueId AddNewNodes(const NodeDescription& nodeDesc, const YamlNode& yamlNde, const NodeUniqueId nodeUid = -1);
    NodeUniqueId RestoreNode(const Node& nodeSnapShot);
    void         HandleDeletingNodes();
    void         DeleteNode(NodeUniqueId nodeUid, bool shouldUnregisterUid);
    

    // handle add/delete edges
    void         HandleAddEdges();
    EdgeUniqueId AddNewEdge(PortUniqueId srcPortUid, PortUniqueId dstPortUid, const YamlEdge& yamlEdge = {},
                    bool avoidMultipleInputLinks = true);
    void               HandleDeletingEdges();
    void               DeleteEdge(EdgeUniqueId edgeUid, bool shouldUnregisterUid);
    void               DeleteEdgesBeforDeleteNode(NodeUniqueId nodeUid, bool shouldUnregisterUid);
    void               DeleteEdgeUidFromPort(EdgeUniqueId edgeUid);
    // handle nodes layout afer toposorted
    void RearrangeNodesLayout(const std::vector<std::vector<NodeUniqueId>>& topologicalOrder,
                              const std::unordered_map<NodeUniqueId, Node>& nodesMap);

    // handle user interactions
    void HandleNodeInfoEditing();
    void HandleEdgeInfoEditing();
    void HandleZooming();
    void HandleOtherUserInputs();

    void               SaveToFile(const std::string& fileName); 
    void               SaveToFile(std::unique_ptr<std::ostream> outputStream);
    [[nodiscard]] bool LoadPipelineFromFile(const std::string& filePath);
    [[nodiscard]] bool LoadPipelineFromStream(std::unique_ptr<std::istream> inputStream);
    void               ClearCurrentPipeLine();

    void               ExecuteCommand(std::unique_ptr<ICommand> cmd);
    bool               Undo();
    bool               Redo();

    // Snapshot and restore methods for undo/redo
    void               RestoreEdge(const Edge& edgeSnapshot);
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

    std::string m_currentPipeLineName;

    ImNodesStyle* m_nodeStyle;

    // serilizer and deserializer
    // should held by nodeeditor instance?
    PipelineEmitter m_pipelineEimtter;
    PipelineParser  m_pipeLineParser;

    FileDialog      m_fileDialog;

    CommandQueue m_commandQueue;

    GraphPruningPolicy m_pruningPolicy;
    bool               m_hideUnlinkedPorts;  // true: hide unlinked ports; false: show all ports

};
} // namespace SimpleNodeEditor

#endif // NODEEDITOR_H