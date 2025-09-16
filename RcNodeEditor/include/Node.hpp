#ifndef NODE_H
#define NODE_H

#include <imgui.h>
#include <stdint.h>

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

template <typename T>
struct UniqueIdGenerator
{
    T m_Uid{};
    T AllocUniqueID()
    {
        return ++m_Uid;
    }
};

class Port
{
public: // type def
    using PortUniqueId =
        int32_t; // used by imnodes, imnode lib need nodeuid to differentiate between nodes
    using PortId =
        int32_t; // normally, every node's inputports/ourports' Id is increased from 0 in steps of 1
    using PortUPtr = std::unique_ptr<Port>;

public:
    Port(PortUniqueId portUid, PortId portId = 0, const std::string& name = "Unknown");
    void SetPortname(const std::string& name);
    void SetPortId(PortId portId);

    std::string_view GetPortname(const std::string& name) const;
    PortId           GetPortId() const;
    PortUniqueId     GetPortUniqueId() const;

private:
    PortUniqueId m_portUid;
    PortId       m_portId;
    std::string  m_portName;
};

class InputPort : public Port
{
public:
    InputPort(PortUniqueId portUid, PortId portId, const std::string& name);

private:
    Port::PortUniqueId m_linkedOutport; // input port have multiple edges
};

class OutputPort : public Port
{
public:
    OutputPort(PortUniqueId portUid, PortId portId, const std::string& name);

private:
    std::vector<Port::PortUniqueId> m_linkedInports; // note that outports only have one edge
};

class Edge
{
public:
    using EdgeUniqueId = int32_t;
    using EdgeUPtr     = std::unique_ptr<Edge>;

public:
    Edge(Port::PortUniqueId inputPortUid, Port::PortUniqueId outputPortUid, EdgeUniqueId edgeUid);
    EdgeUniqueId GetEdgeUniqueId() const;
    Port::PortUniqueId GetInputPortUid() const;
    Port::PortUniqueId GetOutputPortUid() const;
private:
    Port::PortUniqueId m_inputPortUid;
    Port::PortUniqueId m_outputPortUid;
    EdgeUniqueId       m_edgeUid;
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
    Node(NodeUniqueId nodeUid, NodeType nodeType, const std::string& nodeTitle,
         float nodeWidth = 100.f);
    void                           SetNodePosition(const ImVec2& pos);
    void                           AddInputPort(const InputPort& inPort);
    void                           AddOutputPort(const OutputPort& ourPort);
    void                           SetNodeTitle(const std::string& nodeTitle);
    const std::string_view         GetNodeTitle() const;
    NodeUniqueId                   GetNodeUniqueId() const;
    const std::vector<InputPort>&  GetInputPorts() const;
    const std::vector<OutputPort>& GetOutputPorts() const;

private:
    // imnode lib need nodeuid to differentiate between nodes
    NodeUniqueId            m_nodeUid;
    NodeType                m_nodeType;
    float                   m_nodeWidth;
    std::string             m_nodeTitle;
    ImVec2                  m_nodePos;
    std::vector<InputPort>  m_inputPorts;
    std::vector<OutputPort> m_outputPorts;
};

#endif // NODE_H