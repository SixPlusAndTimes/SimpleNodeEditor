#ifndef NODEEDITOR_H
#define NODEEDITOR_H
#include "Node.hpp"

class NodeEditor
{
public:
    NodeEditor();
    void NodeEditorInitialize();
    void NodeEditorShow();
    void NodeEditorDestroy();
    void HandleDeletingNodes();

private:
    void ShowMenu();
    void ShowInfos();
    void HandleAddNodes();
    void ShowNodes();
    void ShowEdges();
    void HandleAddEdges();
    void HandleDeletingEdges();
    void DeleteEdgesBeforDeleteNode(NodeUniqueId nodeUid);

private:
    std::unordered_map<NodeUniqueId, Node> m_nodes; // storage nodes that will be rendered on canvas

    std::unordered_map<EdgeUniqueId, Edge> m_edges;

    std::unordered_map<PortUniqueId, InputPort*>  m_inportPorts;
    std::unordered_map<PortUniqueId, OutputPort*> m_outportPorts;

    // uniqueid generators
    UniqueIdGenerator<NodeUniqueId> m_nodeUidGenerator;
    UniqueIdGenerator<PortUniqueId> m_portUidGenerator;
    UniqueIdGenerator<EdgeUniqueId> m_edgeUidGenerator;
};

#endif // NODEEDITOR_H