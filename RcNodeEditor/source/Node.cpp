#include "Node.hpp"
#include <algorithm>
#include "spdlog/spdlog.h"

Port::Port(PortUniqueId portUid, PortId portId, const std::string& name, NodeUniqueId ownedBy)
    : m_portUid(portUid), m_portId(portId), m_portName(name), m_ownedByNodeUid(ownedBy)
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

std::string_view Port::GetPortname() const
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

NodeUniqueId Port::OwnedByNodeUid() const
{
    return m_ownedByNodeUid;
}

InputPort::InputPort(PortUniqueId portUid, PortId portId, const std::string& name, NodeUniqueId ownedBy)
    : Port(portUid, portId, name, ownedBy), m_linkFrom(-1)
{
    SPDLOG_INFO("InputPort construced with portUid = {}, portId = {}, portName = {}, linkfrom = {}", portUid, portId, name, m_linkFrom);
}


void InputPort::SetEdgeUid(EdgeUniqueId edgeUid)
{
    m_linkFrom = edgeUid;
    SPDLOG_INFO("InputPort id = {}, setedgeuid = {}", GetPortUniqueId(), edgeUid);
}

EdgeUniqueId InputPort::GetEdgeUid()
{
    return m_linkFrom;
}

OutputPort::OutputPort(PortUniqueId portUid, PortId portId, const std::string& name, NodeUniqueId ownedBy)
    : Port(portUid, portId, name, ownedBy), m_linkTos()
{
    SPDLOG_INFO("OutputPort construced with portUid = {}, portId = {}, portName = {}", portUid, portId, name);
}

Edge::Edge(PortUniqueId sourcePortUid, PortUniqueId destinationPortUid, EdgeUniqueId edgeUid)
    : m_srcPortUid(sourcePortUid), m_dstPortUid(destinationPortUid), m_edgeUid(edgeUid)
{
}

Edge::Edge(PortUniqueId sourcePortUid, NodeUniqueId sourceNodeUid, PortUniqueId destinationPortUid, NodeUniqueId destinationNodeUid, EdgeUniqueId edgeUid)
    : m_srcPortUid(sourcePortUid), m_srcNodeUid(sourceNodeUid), m_dstPortUid(destinationPortUid), m_dstNodeUid(destinationNodeUid), m_edgeUid(edgeUid)
{
}
EdgeUniqueId Edge::GetEdgeUniqueId() const
{
    return m_edgeUid;
}

void Edge::SetSourceNodeUid(NodeUniqueId nodeUid)
{
    m_srcNodeUid = nodeUid;
}

void Edge::SetDestinationNodeUid(NodeUniqueId nodeUid)
{
    m_dstNodeUid = nodeUid;
}


PortUniqueId Edge::GetSourcePortUid() const
{
    return m_srcPortUid;
}

PortUniqueId Edge::GetDestinationPortUid() const
{
    return m_dstPortUid;
}

NodeUniqueId Edge::GetSourceNodeUid() const
{
    return m_srcNodeUid;
}

NodeUniqueId Edge::GetDestinationNodeUid() const
{
    return m_dstNodeUid;
}

// Node::Node() : Node(-1, NodeType::Unknown, "Default") {}

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
    SPDLOG_INFO("Node constructed with nodeUid = {}, nodeTtile = {}", nodeUid, nodeTitle);
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

const std::vector<InputPort>& Node::GetInputPorts() const
{
    return m_inputPorts;
}

std::vector<InputPort>& Node::GetInputPorts()
{
    return m_inputPorts;
}

InputPort* Node::GetInputPort(PortUniqueId portUid)
{
    for (auto& inport : m_inputPorts)
    {
        if (inport.GetPortUniqueId() == portUid) 
        {
            return &inport;
        }
    }

    SPDLOG_ERROR("Can not find portUid = {} in Nodeuid = {}, check it!", portUid, m_nodeUid);
    return nullptr;
}

const std::vector<OutputPort>& Node::GetOutputPorts() const
{
    return m_outputPorts;
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

const std::vector<EdgeUniqueId>& OutputPort::GetEdgeUids() const
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
        SPDLOG_ERROR("cannot find outport, check it! portUid = {}", portUid);
        return nullptr;
    }
}
