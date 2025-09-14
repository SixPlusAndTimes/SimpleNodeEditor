#ifndef NODEEDITOR_H
#define NODEEDITOR_H
#include "Node.hpp"

class NodeEditor
{
public:
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
        m_nodes; // port entity is owned by nodes, edge reference port by portuid
    std::unordered_map<Edge::EdgeUniqueId, Edge> m_edges;
};

#endif // NODEEDITOR_H