#ifndef NODEEDITOR_H
#define NODEEDITOR_H
#include "Node.hpp"
#include "imnodes.h"
#include "NodeDescription.hpp"

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
    void            ShowMenu();
    void            ShowInfos();
    void            HandleAddNodes();
    NodeUniqueId    AddNewNodes(const NodeDescription& nodeDesc, YamlNode::NodeYamlId yamlNodeId = -1);
    void            ShowNodes();
    void            ShowEdges();
    void            HandleAddEdges();
    void            AddNewEdge(PortUniqueId srcPortUid, PortUniqueId dstPortUid);
    void            HandleDeletingEdges();
    void            DeleteEdgesBeforDeleteNode(NodeUniqueId nodeUid);
    bool            IsInportAlreadyHasEdge(PortUniqueId portUid);
    void            DeleteEdgeUidFromPort(EdgeUniqueId edgeUid);
    void            SaveState();
private:
    std::unordered_map<NodeUniqueId, Node> m_nodes; // store nodes that will be rendered on canvas

    std::unordered_map<EdgeUniqueId, Edge> m_edges; // store edges that will be rendered on canvas

    std::unordered_map<PortUniqueId, InputPort*>  m_inportPorts; // hold pointers to ports which actually owned by Nodes
    std::unordered_map<PortUniqueId, OutputPort*> m_outportPorts; 

    // uniqueid generators
    UniqueIdGenerator<NodeUniqueId> m_nodeUidGenerator;
    UniqueIdGenerator<PortUniqueId> m_portUidGenerator;
    UniqueIdGenerator<EdgeUniqueId> m_edgeUidGenerator;

    ImNodesMiniMapLocation          m_minimap_location;

    std::vector<NodeDescription>    m_nodeDescriptions;
};
} // namespace SimpleNodeEditor


#endif // NODEEDITOR_H