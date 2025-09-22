#ifndef NODEEDITOR_H
#define NODEEDITOR_H
#include "Node.hpp"

class NodeEditor
{
public:
    NodeEditor();
    // bool AddNode(int a);
    // bool DeleteNode();
    // bool AddEdge();
    // bool DeleteEdge();
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
private:
    std::unordered_map<NodeUniqueId, Node>
        m_nodes; // storage nodes that will be rendered on canvas
    
    std::unordered_map<EdgeUniqueId, Edge> m_edges;

    // uniqueid generators
    UniqueIdGenerator<NodeUniqueId> m_nodeUidGenerator;
    UniqueIdGenerator<PortUniqueId> m_portUidGenerator;
    UniqueIdGenerator<EdgeUniqueId> m_edgeUidGenerator;
};

#endif // NODEEDITOR_H