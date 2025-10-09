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
        return m_Uid++;
    }
};

using PortUniqueId = int32_t; // used by imnodes, imnode lib need nodeuid to differentiate between
                              // nodes links and ports
using EdgeUniqueId = int32_t;
using NodeUniqueId = int32_t;

class Port
{
public: // type def
    using PortId =
        int32_t; // normally, every node's inputports/ourports' Id is increased from 0 in steps of 1
    using PortUPtr = std::unique_ptr<Port>;

public:
    Port(PortUniqueId portUid, PortId portId, const std::string& name, NodeUniqueId ownedBy);
    void SetPortname(const std::string& name);
    void SetPortId(PortId portId);

    std::string_view GetPortname() const;
    PortId           GetPortId() const;
    PortUniqueId     GetPortUniqueId() const;
    NodeUniqueId     OwnedByNodeUid() const; // return the uid of the node that this port belongs to

private:
    PortUniqueId m_portUid;
    PortId       m_portId;
    std::string  m_portName;
    NodeUniqueId m_ownedByNodeUid;  // indicating which node the port belongs to
};

class InputPort : public Port
{
public:
    InputPort(PortUniqueId portUid, PortId portId, const std::string& name, NodeUniqueId ownedBy);
    void         SetEdgeUid(EdgeUniqueId);
    EdgeUniqueId GetEdgeUid();

private:
    EdgeUniqueId m_linkFrom;
};

class OutputPort : public Port
{
public:
    OutputPort(PortUniqueId portUid, PortId portId, const std::string& name, NodeUniqueId ownedBy);
    void                       PushEdge(EdgeUniqueId);
    void                       DeletEdge(EdgeUniqueId);
    std::vector<EdgeUniqueId>& GetEdgeUids();
    void                       ClearEdges();

private:
    std::vector<EdgeUniqueId> m_linkTos; // outports have multiple edges
};

class Edge
{
public:
    using EdgeUPtr = std::unique_ptr<Edge>;

public:
    Edge(PortUniqueId sourcePortUid, PortUniqueId destinationPortUid, EdgeUniqueId edgeUid);
    Edge(PortUniqueId sourcePortUid, NodeUniqueId sourceNodeUid, PortUniqueId destinationPortUid, NodeUniqueId destinationNodeUid, EdgeUniqueId edgeUid);
    EdgeUniqueId GetEdgeUniqueId() const;
    PortUniqueId GetSourcePortUid() const;
    PortUniqueId GetDestinationPortUid() const;
    NodeUniqueId GetSourceNodeUid() const;
    NodeUniqueId GetDestinationNodeUid() const;

private:
    PortUniqueId m_srcPortUid;
    NodeUniqueId m_srcNodeUid;
    PortUniqueId m_dstPortUid;
    NodeUniqueId m_dstNodeUid;
    EdgeUniqueId m_edgeUid;
};

class Node
{
public: // type def
    enum class NodeType
    {
        SourceNode,
        NormalNode,
        SinkNode,
        Unknown
    };
    using NodeUPtr   = std::unique_ptr<Node>;
    using NodeYamlId = int32_t;

public:
    Node();
    Node(NodeUniqueId nodeUid, NodeType nodeType, const std::string& nodeTitle,
         float nodeWidth = 100.f);
    void                     SetNodePosition(const ImVec2& pos);
    void                     AddInputPort(const InputPort& inPort);
    void                     AddOutputPort(const OutputPort& ourPort);
    void                     SetNodeTitle(const std::string& nodeTitle);
    const std::string_view   GetNodeTitle() const;
    NodeUniqueId             GetNodeUniqueId() const;
    std::vector<InputPort>&  GetInputPorts();
    std::vector<OutputPort>& GetOutputPorts();
    InputPort*               GetInputPort(PortUniqueId portUid);
    OutputPort*              GetOutputPort(PortUniqueId portUid);

private:
    // imnode lib need nodeuid to differentiate between nodes
    NodeUniqueId            m_nodeUid;
    NodeType                m_nodeType;
    NodeYamlId              m_nodeYamlId;
    float                   m_nodeWidth;
    std::string             m_nodeTitle;
    ImVec2                  m_nodePos;
    std::vector<InputPort>  m_inputPorts;
    std::vector<OutputPort> m_outputPorts;
};

#endif // NODE_H