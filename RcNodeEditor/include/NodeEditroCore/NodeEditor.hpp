#ifndef NODEEDITOR_H
#define NODEEDITOR_H
#include <imgui.h>
#include <stdint.h>

#include <memory>
#include <unordered_map>
#include <vector>

class Edge;

class Port
{
public: // type def
    using PortUniqueId = int32_t;
    using PortUPtr     = std::unique_ptr<Port>;

public:
private:
    std::string         m_pinName;
    static PortUniqueId m_portUid; // imnode lib need nodeuid to differentiate between nodes
};

class InputPort : public Port
{
public:
private:
    PortUniqueId       m_uid;
    Edge::EdgeUniqueId m_inputEdge; // input pins only have one edge
};

class OutputPort : public Port
{
public:
private:
    PortUniqueId                    m_uid;
    std::vector<Edge::EdgeUniqueId> m_outputEdges; // output pins have multiple edges
};

class Edge
{
public:
    using EdgeUniqueId = int32_t;
    using EdgeUPtr     = std::unique_ptr<Edge>;

private:
    Port::PortUniqueId m_inputPortId;
    Port::PortUniqueId m_outputPortId;
    EdgeUniqueId       m_edgeIid;
};

class Node
{
public: // type def
    enum class NodeType
    {
        SourceNode,
        NormalNode,
        SinkNode
    };
    using NodeUniqueId = int32_t;
    using NodeUPtr     = std::unique_ptr<Node>;

public:
private:
    NodeUniqueId            m_nodeUid; // imnode lib need nodeuid to differentiate between nodes
    NodeType                m_nodeType;
    std::string             m_nodeTitle;
    ImVec2                  m_nodePos;
    float                   m_nodeWidth;
    std::vector<InputPort>  m_inputPorts;
    std::vector<OutputPort> m_outputPorts;
};

class NodeEditor
{
public:
    bool AddNode(int a);
    bool DeleteNode();
    bool AddEdge();
    bool DeleteEdge();

private:
    std::unordered_map<Node::NodeUniqueId, Node> m_nodes;
    std::unordered_map<Edge::EdgeUniqueId, Edge> m_edges;
};

#endif // NODEEDITOR_H