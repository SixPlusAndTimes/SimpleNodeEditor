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
private:
    void ShowMenu();
    void ShowInfos();
    void HandleAddNodes();
    void ShowNodes();
    // void ShowEdges();
    // void HandleAddEdges();
    // void HandleDeletedEdges();
private:
    std::unordered_map<Node::NodeUniqueId, Node>
        m_nodes; // storage nodes that will be rendered on canvas
    
    std::unordered_map<Edge::EdgeUniqueId, Edge> m_edges;

    // uniqueid generators
    UniqueIdGenerator<Node::NodeUniqueId> m_nodeUidGenerator;
    UniqueIdGenerator<Port::PortUniqueId> m_portUidGenerator;
    UniqueIdGenerator<Edge::EdgeUniqueId> m_edgeUidGenerator;
};

#endif // NODEEDITOR_H