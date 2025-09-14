#include "Node.hpp"

Port::Port(PortUniqueId portUid, PortId portId, const std::string& name)
    : m_portUid(portId), m_portId(portUid), m_portName(name)
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

Port::PortId Port::GetPortId(PortId portId) const
{
    return m_portId;
}

Port::PortUniqueId Port::GetPortUniqueId(PortUniqueId portUid) const
{
    return m_portUid;
}

InputPort::InputPort(PortUniqueId portUid, PortId portId, const std::string& name)
    : Port(portUid, portId, name), m_linkedOutport(-1)
{
}

OutputPort::OutputPort(PortUniqueId portUid, PortId portId, const std::string& name)
    : Port(portUid, portId, name), m_linkedInports()
{
}

Node::Node(NodeUniqueId nodeUid, NodeType nodeType, const std::string& nodeTitle, float nodeWidth)
    : m_nodeUid(nodeUid),
      m_nodeType(nodeType),
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

Node::NodeUniqueId Node::GetNodeUniqueId() const
{
    return m_nodeUid;
}

void Node::SetNodeTitle(const std::string& nodeTitle)
{
    m_nodeTitle = nodeTitle;
}

std::string_view Node::GetNodeTitle()
{
    return m_nodeTitle;
}
