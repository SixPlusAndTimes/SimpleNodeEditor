#ifndef NODE_H
#define NODE_H

#include <imgui.h>
#include <stdint.h>

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

namespace SimpleNodeEditor
{
    
template <typename T>
struct UniqueIdGenerator
{
    T m_Uid{};
    T AllocUniqueID()
    {
        return m_Uid++;
    }
};

// used by imnodes, imnode lib need nodeuid to differentiate between
// nodes links and ports
using PortUniqueId = int32_t; 
using EdgeUniqueId = int32_t;
using NodeUniqueId = int32_t;

struct YamlPropertyDescription
{
    std::string m_propertyName;
    std::string m_propertyValue;
};

struct YamlNode
{
    using NodeYamlId = int32_t;

    YamlNode() : m_nodeName("Unknown"), m_nodeYamlId(-1), m_isSrcNode(false), m_Properties()
    { }
    std::string                             m_nodeName;
    NodeYamlId                              m_nodeYamlId;
    bool                                    m_isSrcNode;
    std::vector<YamlPropertyDescription>    m_Properties;
};


struct YamlPort
{
    using PortYamlId = int32_t;
    std::string             m_nodeName;
    YamlNode::NodeYamlId    m_nodeYamlId;
    std::string             m_portName;
    PortYamlId              m_portYamlId;
    
};

// srcport info maybe redundant in different edges, check it later
struct YamlEdge
{
    YamlPort m_yamlSrcPort;
    YamlPort m_yamlDstPort;
    // TODO : add properties
};

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
    const std::vector<EdgeUniqueId>& GetEdgeUids() const;
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
    void SetSourceNodeUid(NodeUniqueId nodeUid);
    void SetDestinationNodeUid(NodeUniqueId nodeUid);

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

public:
    // Node();
    Node(NodeUniqueId nodeUid, NodeType nodeType, const std::string& nodeTitle,
         float nodeWidth = 100.f);
    Node(YamlNode::NodeYamlId nodeYamlId, NodeType nodeType, float nodeWidth = 100.f);
    void                     SetNodePosition(const ImVec2& pos);

    void                     AddInputPort(const InputPort& inPort);
    void                     AddOutputPort(const OutputPort& ourPort);
    void                     SetNodeTitle(const std::string& nodeTitle);
    const std::string_view   GetNodeTitle() const;
    NodeUniqueId             GetNodeUniqueId() const;
    const std::vector<InputPort>&  GetInputPorts() const;
    const std::vector<OutputPort>& GetOutputPorts() const;
    std::vector<InputPort>&  GetInputPorts();
    std::vector<OutputPort>& GetOutputPorts();
    InputPort*               GetInputPort(PortUniqueId portUid);
    OutputPort*              GetOutputPort(PortUniqueId portUid);
    
    // yaml node related
    void                     SetNodeYamlId(YamlNode::NodeYamlId nodeYamlId);
    YamlNode::NodeYamlId     GetNodeYamlId() const;
private:
    // imnode lib need nodeuid to differentiate between nodes
    NodeUniqueId            m_nodeUid;   // used for imnode to draw UI
    NodeType                m_nodeType;
    float                   m_nodeWidth;
    std::string             m_nodeTitle; // nodetitle = nodename_in_nodedescription +  "_" + nodeid_in_yaml
    ImVec2                  m_nodePos;
    std::vector<InputPort>  m_inputPorts;
    std::vector<OutputPort> m_outputPorts;

    // yaml node
    YamlNode                m_yamlNodeDescription;
};
} // namespace SimpleNodeEditor


#endif // NODE_H