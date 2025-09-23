#include "Node.hpp"
#include <algorithm>
#include "spdlog/spdlog.h"

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
    : m_inputPortUid(inputPortUid), m_outputPortUid(outputPortUid), m_edgeUid(edgeUid)
{
}

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

Node::Node() : Node(-1, NodeType::Unknown, "Default") {}

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

std::vector<InputPort>& Node::GetInputPorts()
{
    return m_inputPorts;
}

InputPort* Node::GetInputPort(PortUniqueId portUid)
{
    auto iter =
        std::find_if(m_inputPorts.begin(), m_inputPorts.end(), [portUid](const InputPort& inPort)
                     { return portUid == inPort.GetPortUniqueId(); });

    if (iter != m_inputPorts.end())
    {
        return &(*iter);
    }
    else
    {
        return nullptr;
    }
}

void InputPort::SetEdgeUid(EdgeUniqueId edgeUid)
{
    m_linkFrom = edgeUid;
}

EdgeUniqueId InputPort::GetEdgeUid()
{
    return m_linkFrom;
}

std::vector<OutputPort>& Node::GetOutputPorts()
{
    return m_outputPorts;
}

void OutputPort::PushEdge(EdgeUniqueId edgeUid)
{
    SPDLOG_INFO("outport id[{}], push edge id[{}]", GetPortUniqueId(), edgeUid);
    m_linkTos.push_back(edgeUid);
}

void OutputPort::DeletEdge(EdgeUniqueId edgeUid)
{
    auto iter = std::find_if(m_linkTos.begin(), m_linkTos.end(), 
                            [edgeUid](EdgeUniqueId traverseEle) { return edgeUid == traverseEle; });

    if (iter != m_linkTos.end())
    {
        m_linkTos.erase(iter);
    }
}

std::vector<EdgeUniqueId>& OutputPort::GetEdgeUids()
{
    return m_linkTos;
}

void OutputPort::ClearEdges()
{
    std::vector<EdgeUniqueId>().swap(m_linkTos);
}

OutputPort* Node::GetOutputPort(PortUniqueId portUid)
{
    auto iter = std::find_if(m_outputPorts.begin(), m_outputPorts.end(),
                             [portUid](const OutputPort& outPort)
                             { return portUid == outPort.GetPortUniqueId(); });

    if (iter != m_outputPorts.end())
    {
        return &(*iter);
    }
    else
    {
        return nullptr;
    }
}
