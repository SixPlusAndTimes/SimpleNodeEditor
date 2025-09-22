#include "Node.hpp"

Port::Port(PortUniqueId portUid, PortId portId, const std::string& name)
    : m_portUid(portUid), m_portId(portId), m_portName(name)
{
}

void Port::SetPortId(PortId portId)
{
    m_portId = portId;
}
void Port::SetPortname(const std::string& name)
{
    m_portName = name;
}

std::string_view Port::GetPortname(const std::string& name) const
{
    return m_portName;
}

Port::PortId Port::GetPortId() const
{
    return m_portId;
}

PortUniqueId Port::GetPortUniqueId() const
{
    return m_portUid;
}

InputPort::InputPort(PortUniqueId portUid, PortId portId, const std::string& name)
    : Port(portUid, portId, name), m_linkFrom(-1)
{
}

OutputPort::OutputPort(PortUniqueId portUid, PortId portId, const std::string& name)
    : Port(portUid, portId, name), m_linkTos()
{
}

Edge::Edge(PortUniqueId inputPortUid, PortUniqueId outputPortUid, EdgeUniqueId edgeUid)
: m_inputPortUid(inputPortUid)
, m_outputPortUid(outputPortUid)
, m_edgeUid(edgeUid)
{ }

EdgeUniqueId Edge::GetEdgeUniqueId() const
{
    return m_edgeUid;
}

PortUniqueId Edge::GetInputPortUid() const
{
    return m_inputPortUid;
}

PortUniqueId Edge::GetOutputPortUid() const
{
    return m_outputPortUid;
}


Node::Node(NodeUniqueId nodeUid, NodeType nodeType, const std::string& nodeTitle, float nodeWidth)
    : m_nodeUid(nodeUid),
      m_nodeType(nodeType),
      m_nodeYamlId(-1),
      m_nodeWidth(nodeWidth),
      m_nodeTitle(nodeTitle),
      m_nodePos(),
      m_inputPorts(),
      m_outputPorts()
{
}

void Node::SetNodePosition(const ImVec2& pos)
{
    m_nodePos = pos;
}

void Node::AddInputPort(const InputPort& inPort)
{
    m_inputPorts.push_back(inPort);
}

void Node::AddOutputPort(const OutputPort& outPort)
{
    m_outputPorts.push_back(outPort);
}

NodeUniqueId Node::GetNodeUniqueId() const
{
    return m_nodeUid;
}

void Node::SetNodeTitle(const std::string& nodeTitle)
{
    m_nodeTitle = nodeTitle;
}

const std::string_view Node::GetNodeTitle() const
{
    return m_nodeTitle;
}

const std::vector<InputPort>&  Node::GetInputPorts() const
{
    return m_inputPorts;
}

const std::vector<OutputPort>& Node::GetOutputPorts() const
{
    return m_outputPorts;
}
